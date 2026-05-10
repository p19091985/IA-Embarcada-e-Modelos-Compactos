#!/usr/bin/env python3
"""
Testes de quantizacao e dequantizacao.
Verifica que o round-trip quantizar -> dequantizar preserva os valores
dentro de tolerancia aceitavel para o pipeline quantizado do projeto.
"""

import pathlib
import unittest

import numpy as np

ROOT = pathlib.Path(__file__).resolve().parents[1]


def quantizar_int8(valor, scale, zero_point):
    """Replica a logica de quantizar_int8() do soil_tflite_runner.cc"""
    quantizado = int(round(valor / scale + zero_point))
    return max(-128, min(127, quantizado))


def quantizar_uint8(valor, scale, zero_point):
    """Replica a logica de quantizar_uint8() do soil_tflite_runner.cc"""
    quantizado = int(round(valor / scale + zero_point))
    return max(0, min(255, quantizado))


def dequantizar(valor_quantizado, scale, zero_point):
    """Dequantiza um valor int8/uint8 de volta para float."""
    return (valor_quantizado - zero_point) * scale


class TestQuantizacaoInt8(unittest.TestCase):
    def test_round_trip_zero(self):
        """Quantizar 0.0 e dequantizar deve retornar ~0.0"""
        scale = 0.01
        zp = 0
        q = quantizar_int8(0.0, scale, zp)
        resultado = dequantizar(q, scale, zp)
        self.assertAlmostEqual(resultado, 0.0, delta=scale)

    def test_round_trip_positivo(self):
        """Quantizar valor positivo e dequantizar deve preservar aproximadamente"""
        scale = 0.05
        zp = -128
        for valor in [0.0, 0.5, 1.0, 0.25, 0.75]:
            q = quantizar_int8(valor, scale, zp)
            resultado = dequantizar(q, scale, zp)
            self.assertAlmostEqual(resultado, valor, delta=scale * 2,
                                   msg=f"Round-trip falhou para valor={valor}")

    def test_clamp_overflow_int8(self):
        """Valores fora do range devem ser clampados"""
        scale = 0.01
        zp = 0
        self.assertEqual(quantizar_int8(999.0, scale, zp), 127)
        self.assertEqual(quantizar_int8(-999.0, scale, zp), -128)

    def test_clamp_overflow_uint8(self):
        """Valores fora do range devem ser clampados"""
        scale = 0.01
        zp = 128
        self.assertEqual(quantizar_uint8(999.0, scale, zp), 255)
        self.assertEqual(quantizar_uint8(-999.0, scale, zp), 0)


class TestQuantizacaoUint8(unittest.TestCase):
    def test_round_trip_uint8(self):
        """Quantizar para uint8 e dequantizar deve preservar valor"""
        scale = 0.04
        zp = 128
        for valor in [0.0, 0.5, 1.0]:
            q = quantizar_uint8(valor, scale, zp)
            resultado = dequantizar(q, scale, zp)
            self.assertAlmostEqual(resultado, valor, delta=scale * 2,
                                   msg=f"Round-trip uint8 falhou para valor={valor}")


class TestQuantizacaoVetorial(unittest.TestCase):
    def test_normalizacao_e_quantizacao_pipeline(self):
        """Testa o pipeline completo: normalizar -> quantizar -> dequantizar"""
        # Simular features do dataset Wazihub
        feat_min = np.array([0.0, 0.0, 11.22, 0.59, 100.5, 0.0, 0.0, 0.0, 1.0], dtype=np.float32)
        feat_max = np.array([88.0, 1.0, 45.56, 96.0, 101.86, 31.36, 133.33, 337.5, 4.0], dtype=np.float32)
        features = np.array([50.0, 1.0, 25.0, 55.0, 101.0, 5.0, 20.0, 180.0, 2.0], dtype=np.float32)

        # Normalizar (min-max para [0, 1])
        span = feat_max - feat_min
        span[span < 1e-6] = 1.0
        normalized = np.clip((features - feat_min) / span, 0.0, 1.0)

        # Quantizar e dequantizar cada feature
        scale = 0.00784  # escala tipica para range [0, 1] em int8
        zp = -128
        for i, val in enumerate(normalized):
            q = quantizar_int8(float(val), scale, zp)
            deq = dequantizar(q, scale, zp)
            self.assertAlmostEqual(deq, float(val), delta=0.02,
                                   msg=f"Pipeline falhou na feature {i}")


if __name__ == "__main__":
    unittest.main()
