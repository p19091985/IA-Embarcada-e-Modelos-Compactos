#!/usr/bin/env python3
"""
Testes de inferencia com o modelo TFLite embarcado.
Carrega o modelo int8, alimenta com amostras do dataset Wazihub
e verifica que as predicoes estao no range correto.
"""

import csv
import pathlib
import unittest

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]
DATASET = ROOT / "datasets" / "Wazihub_Soil_Moisture_Prediction" / "Dataset" / "Train.csv"
INT8_MODEL = ROOT / "main" / "models" / "soil_moisture_level_int8.tflite"
FLOAT_MODEL = ROOT / "main" / "models" / "soil_moisture_level_float.tflite"

# Features usadas pelo modelo (mesma ordem do soil_model_metadata.h)
FEATURE_COLUMNS = [
    "Soil humidity 1",
    "Irrigation field 1",
    "Air temperature (C)",
    "Air humidity (%)",
    "Pressure (KPa)",
    "Wind speed (Km/h)",
    "Wind gust (Km/h)",
    "Wind direction (Deg)",
]

# Constantes do firmware (de soil_model_metadata.h)
FEAT_MIN = np.array([0.0, 0.0, 11.22, 0.59, 100.5, 0.0, 0.0, 0.0, 1.0], dtype=np.float32)
FEAT_MAX = np.array([88.0, 1.0, 45.56, 96.0, 101.86, 31.36, 133.33, 337.5, 4.0], dtype=np.float32)
FIELD_ID = 1
INPUT_COUNT = 9


def carregar_amostras(n=200):
    """Carrega N amostras do dataset para teste."""
    rows = []
    with DATASET.open(newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                feats = [float(row[col]) for col in FEATURE_COLUMNS]
                feats.append(float(FIELD_ID))
                rows.append(feats)
            except (ValueError, KeyError):
                continue
            if len(rows) >= n:
                break
    return np.array(rows, dtype=np.float32)


def normalizar(X):
    """Normaliza features para [0, 1] usando min-max do firmware."""
    span = FEAT_MAX - FEAT_MIN
    span[span < 1e-6] = 1.0
    return np.clip((X - FEAT_MIN) / span, 0.0, 1.0).astype(np.float32)


def inferir(model_path, X_norm):
    """Faz inferencia com o modelo TFLite."""
    try:
        import tensorflow as tf
        interpreter = tf.lite.Interpreter(model_path=str(model_path))
    except ImportError:
        # fallback: tflite_runtime
        import tflite_runtime.interpreter as tflite
        interpreter = tflite.Interpreter(model_path=str(model_path))

    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    predictions = []
    for sample in X_norm:
        input_data = sample.reshape(input_details["shape"])

        # Quantizar entrada se necessario
        quant = input_details.get("quantization", (0.0, 0))
        scale, zp = quant
        if input_details["dtype"] in (np.int8, np.uint8) and scale not in (0, 0.0):
            input_data = np.round(input_data / scale + zp)
            info = np.iinfo(input_details["dtype"])
            input_data = np.clip(input_data, info.min, info.max).astype(input_details["dtype"])
        else:
            input_data = input_data.astype(input_details["dtype"])

        interpreter.set_tensor(input_details["index"], input_data)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details["index"])

        # Dequantizar saida se necessario
        out_val = np.reshape(output, -1)[0]
        out_quant = output_details.get("quantization", (0.0, 0))
        out_scale, out_zp = out_quant
        if output_details["dtype"] in (np.int8, np.uint8) and out_scale not in (0, 0.0):
            out_val = (out_val - out_zp) * out_scale

        predictions.append(float(out_val))

    return np.array(predictions, dtype=np.float32)


class TestInferenciaModelo(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not DATASET.exists():
            raise unittest.SkipTest("Dataset nao encontrado")
        cls.X_raw = carregar_amostras(200)
        cls.X_norm = normalizar(cls.X_raw)

    def test_modelo_int8_existe(self):
        self.assertTrue(INT8_MODEL.exists(), f"Modelo int8 ausente: {INT8_MODEL}")

    def test_modelo_float_existe(self):
        self.assertTrue(FLOAT_MODEL.exists(), f"Modelo float ausente: {FLOAT_MODEL}")

    def test_predicoes_int8_no_range(self):
        """Predicoes do modelo int8 devem estar no range [0, 9]"""
        try:
            preds = inferir(INT8_MODEL, self.X_norm)
        except ImportError:
            self.skipTest("tensorflow ou tflite_runtime nao instalado")

        for i, p in enumerate(preds):
            self.assertGreaterEqual(p, -1.0,
                                    msg=f"Predicao {i} abaixo do range: {p}")
            self.assertLessEqual(p, 10.0,
                                 msg=f"Predicao {i} acima do range: {p}")

    def test_predicoes_float_no_range(self):
        """Predicoes do modelo float devem estar no range [0, 9]"""
        try:
            preds = inferir(FLOAT_MODEL, self.X_norm)
        except ImportError:
            self.skipTest("tensorflow ou tflite_runtime nao instalado")

        for i, p in enumerate(preds):
            self.assertGreaterEqual(p, -1.0,
                                    msg=f"Predicao {i} abaixo do range: {p}")
            self.assertLessEqual(p, 10.0,
                                 msg=f"Predicao {i} acima do range: {p}")

    def test_mae_modelo_int8_aceitavel(self):
        """MAE do modelo int8 deve ser menor que 1.0 (nivel de umidade)"""
        try:
            preds = inferir(INT8_MODEL, self.X_norm)
        except ImportError:
            self.skipTest("tensorflow ou tflite_runtime nao instalado")

        # Calcular nivel real (mesmo que o firmware)
        umidade = self.X_raw[:, 0]
        nivel_real = np.clip(umidade / 88.0 * 9.0, 0.0, 9.0)

        mae = float(np.mean(np.abs(preds - nivel_real)))
        self.assertLess(mae, 1.0,
                        msg=f"MAE do modelo int8 muito alto: {mae:.4f}")

    def test_consistencia_float_vs_int8(self):
        """Diferenca media entre float e int8 deve ser pequena"""
        try:
            preds_float = inferir(FLOAT_MODEL, self.X_norm)
            preds_int8 = inferir(INT8_MODEL, self.X_norm)
        except ImportError:
            self.skipTest("tensorflow ou tflite_runtime nao instalado")

        diff = float(np.mean(np.abs(preds_float - preds_int8)))
        self.assertLess(diff, 0.5,
                        msg=f"Diferenca float vs int8 muito alta: {diff:.4f}")


if __name__ == "__main__":
    unittest.main()
