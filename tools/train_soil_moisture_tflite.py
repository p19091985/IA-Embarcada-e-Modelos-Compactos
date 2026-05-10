#!/usr/bin/env python3
"""Train a soil-moisture TFLite model and export it for ESP-IDF.

The training pipeline uses the Wazihub soil-moisture dataset and generates
firmware-ready C++ files for the embedded irrigation monitor.
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile
from dataclasses import dataclass

import numpy as np
import pandas as pd
import tensorflow as tf


REPO_URL = "https://github.com/ayamlearning/Wazihub_Soil_Moisture_Prediction.git"
REPO_ZIP_URL = "https://github.com/ayamlearning/Wazihub_Soil_Moisture_Prediction/archive/refs/heads/master.zip"
FEATURE_NAMES = [
    "sensor_humidity",
    "irrigation",
    "air_temperature",
    "air_humidity",
    "pressure",
    "wind_speed",
    "wind_gust",
    "wind_direction",
    "field_id",
]
WEATHER_COLUMNS = [
    "Air temperature (C)",
    "Air humidity (%)",
    "Pressure (KPa)",
    "Wind speed (Km/h)",
    "Wind gust (Km/h)",
    "Wind direction (Deg)",
]
FIELD_COLUMNS = [
    ("Soil humidity 1", "Irrigation field 1", 1.0),
    ("Soil humidity 2", "Irrigation field 2", 2.0),
    ("Soil humidity 3", "Irrigation field 3", 3.0),
    ("Soil humidity 4", "Irrigation field 4", 4.0),
]


@dataclass
class PreparedData:
    x_train: np.ndarray
    y_train: np.ndarray
    x_val: np.ndarray
    y_val: np.ndarray
    feature_min: np.ndarray
    feature_max: np.ndarray
    feature_defaults: np.ndarray
    sample_count: int


def ensure_dataset(dataset_dir: pathlib.Path) -> pathlib.Path:
    csv_path = dataset_dir / "Dataset" / "Train.csv"
    if csv_path.exists():
        return csv_path

    dataset_dir.parent.mkdir(parents=True, exist_ok=True)
    try:
        subprocess.run(
            ["git", "clone", "--depth", "1", REPO_URL, str(dataset_dir)],
            check=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        if dataset_dir.exists():
            shutil.rmtree(dataset_dir)
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = pathlib.Path(tmp)
            zip_path = tmp_path / "dataset.zip"
            urllib.request.urlretrieve(REPO_ZIP_URL, zip_path)
            with zipfile.ZipFile(zip_path) as archive:
                archive.extractall(tmp_path)
            extracted_dirs = [p for p in tmp_path.iterdir() if p.is_dir()]
            if not extracted_dirs:
                raise FileNotFoundError("Dataset não encontrado no arquivo ZIP baixado.")
            shutil.move(str(extracted_dirs[0]), str(dataset_dir))

    if not csv_path.exists():
        raise FileNotFoundError(f"Dataset não encontrado: {csv_path}")
    return csv_path


def build_supervised_rows(csv_path: pathlib.Path, horizon_steps: int) -> pd.DataFrame:
    df = pd.read_csv(csv_path)
    frames: list[pd.DataFrame] = []

    for soil_col, irrigation_col, field_id in FIELD_COLUMNS:
        field_df = df[[soil_col, irrigation_col, *WEATHER_COLUMNS]].copy()
        field_df["target_humidity"] = df[soil_col].shift(-horizon_steps)
        field_df["field_id"] = field_id
        field_df = field_df.rename(
            columns={
                soil_col: "sensor_humidity",
                irrigation_col: "irrigation",
                "Air temperature (C)": "air_temperature",
                "Air humidity (%)": "air_humidity",
                "Pressure (KPa)": "pressure",
                "Wind speed (Km/h)": "wind_speed",
                "Wind gust (Km/h)": "wind_gust",
                "Wind direction (Deg)": "wind_direction",
            }
        )
        frames.append(field_df)

    data = pd.concat(frames, ignore_index=True)
    data = data.dropna().reset_index(drop=True)

    data["sensor_humidity"] = data["sensor_humidity"].clip(0.0, 100.0)
    data["target_humidity"] = data["target_humidity"].clip(0.0, 100.0)
    data["target_level"] = (data["target_humidity"] / 100.0) * 9.0
    return data


def prepare_data(csv_path: pathlib.Path, horizon_steps: int, validation_ratio: float) -> PreparedData:
    data = build_supervised_rows(csv_path, horizon_steps)
    features = data[FEATURE_NAMES].astype(np.float32).to_numpy()
    target = data["target_level"].astype(np.float32).to_numpy().reshape(-1, 1)

    feature_min = features.min(axis=0)
    feature_max = features.max(axis=0)
    feature_defaults = np.median(features, axis=0)
    span = np.where((feature_max - feature_min) < 1e-6, 1.0, feature_max - feature_min)
    features_norm = np.clip((features - feature_min) / span, 0.0, 1.0).astype(np.float32)

    rng = np.random.default_rng(42)
    indices = rng.permutation(len(features_norm))
    val_size = int(len(indices) * validation_ratio)
    val_indices = indices[:val_size]
    train_indices = indices[val_size:]

    return PreparedData(
        x_train=features_norm[train_indices],
        y_train=target[train_indices],
        x_val=features_norm[val_indices],
        y_val=target[val_indices],
        feature_min=feature_min,
        feature_max=feature_max,
        feature_defaults=feature_defaults,
        sample_count=len(features_norm),
    )


def create_model(input_count: int) -> tf.keras.Model:
    model = tf.keras.Sequential(
        [
            tf.keras.layers.Input(shape=(input_count,)),
            tf.keras.layers.Dense(16, activation="relu"),
            tf.keras.layers.Dense(16, activation="relu"),
            tf.keras.layers.Dense(1),
        ]
    )
    model.compile(optimizer="adam", loss="mse", metrics=["mae"])
    return model


def train_model(data: PreparedData, epochs: int, batch_size: int) -> tf.keras.Model:
    model = create_model(data.x_train.shape[1])
    model.fit(
        data.x_train,
        data.y_train,
        epochs=epochs,
        batch_size=batch_size,
        validation_data=(data.x_val, data.y_val),
        verbose=2,
    )
    return model


def convert_float_model(model: tf.keras.Model) -> bytes:
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    return converter.convert()


def convert_quantized_model(model: tf.keras.Model, representative_x: np.ndarray) -> bytes:
    def representative_dataset():
        for row in representative_x[:600]:
            yield [row.reshape(1, -1).astype(np.float32)]

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8
    converter.representative_dataset = representative_dataset
    return converter.convert()


def run_tflite(model_bytes: bytes, x: np.ndarray) -> np.ndarray:
    interpreter = tf.lite.Interpreter(model_content=model_bytes)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]
    predictions = np.empty((x.shape[0], 1), dtype=np.float32)

    for i, row in enumerate(x):
        input_data = row.reshape(1, -1).astype(input_details["dtype"])
        scale, zero_point = input_details.get("quantization", (0.0, 0))
        if input_details["dtype"] in (np.int8, np.uint8) and scale:
            input_data = np.round(row.reshape(1, -1) / scale + zero_point)
            dtype_info = np.iinfo(input_details["dtype"])
            input_data = np.clip(input_data, dtype_info.min, dtype_info.max).astype(input_details["dtype"])

        interpreter.set_tensor(input_details["index"], input_data)
        interpreter.invoke()
        output = interpreter.get_tensor(output_details["index"])

        out_scale, out_zero_point = output_details.get("quantization", (0.0, 0))
        if output_details["dtype"] in (np.int8, np.uint8) and out_scale:
            predictions[i, 0] = (float(output.reshape(-1)[0]) - out_zero_point) * out_scale
        else:
            predictions[i, 0] = float(output.reshape(-1)[0])

    return predictions


def regression_metrics(y_true: np.ndarray, y_pred: np.ndarray) -> dict[str, float]:
    errors = y_pred - y_true
    return {
        "mae": float(np.mean(np.abs(errors))),
        "rmse": float(np.sqrt(np.mean(np.square(errors)))),
    }


def write_model_data(model_bytes: bytes, output_cc: pathlib.Path) -> None:
    output_cc.parent.mkdir(parents=True, exist_ok=True)
    hex_values = [f"0x{byte:02x}" for byte in model_bytes]
    lines = []
    for offset in range(0, len(hex_values), 12):
        lines.append("    " + ", ".join(hex_values[offset : offset + 12]) + ",")

    output_cc.write_text(
        "#include \"soil_model_data.h\"\n\n"
        "alignas(16) const unsigned char g_soil_moisture_model[] = {\n"
        + "\n".join(lines)
        + "\n};\n"
        f"const int g_soil_moisture_model_len = {len(model_bytes)};\n",
        encoding="utf-8",
    )


def c_float_array(values: np.ndarray) -> str:
    formatted = []
    for value in values:
        text = f"{float(value):.8g}"
        if "." not in text and "e" not in text.lower():
            text += ".0"
        formatted.append(f"{text}f")
    return ", ".join(formatted)


def write_metadata(data: PreparedData, metrics: dict[str, dict[str, float]], output_h: pathlib.Path, horizon_minutes: int) -> None:
    output_h.write_text(
        "#pragma once\n\n"
        "#include \"soil_app_logic.h\"\n\n"
        "#define SOIL_MODEL_NAME \"soil_moisture_level_int8\"\n"
        f"#define SOIL_MODEL_HORIZON_MINUTES {horizon_minutes}\n"
        f"#define SOIL_MODEL_SAMPLE_COUNT {data.sample_count}\n"
        f"#define SOIL_MODEL_FLOAT_MAE {metrics['float']['mae']:.8g}f\n"
        f"#define SOIL_MODEL_INT8_MAE {metrics['int8']['mae']:.8g}f\n\n"
        "static const float kSoilFeatureMin[SOIL_MODEL_INPUT_COUNT] = {\n"
        f"    {c_float_array(data.feature_min)},\n"
        "};\n\n"
        "static const float kSoilFeatureMax[SOIL_MODEL_INPUT_COUNT] = {\n"
        f"    {c_float_array(data.feature_max)},\n"
        "};\n\n"
        "static const float kSoilFeatureDefaults[SOIL_MODEL_INPUT_COUNT] = {\n"
        f"    {c_float_array(data.feature_defaults)},\n"
        "};\n\n"
        "static const char *const kSoilFeatureNames[SOIL_MODEL_INPUT_COUNT] = {\n"
        + "".join(f"    \"{name}\",\n" for name in FEATURE_NAMES)
        + "};\n",
        encoding="utf-8",
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset-dir", default="datasets/Wazihub_Soil_Moisture_Prediction")
    parser.add_argument("--models-dir", default="main/models")
    parser.add_argument("--main-dir", default="main")
    parser.add_argument("--epochs", type=int, default=80)
    parser.add_argument("--batch-size", type=int, default=128)
    parser.add_argument("--horizon-minutes", type=int, default=60)
    parser.add_argument("--validation-ratio", type=float, default=0.2)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    root = pathlib.Path.cwd()
    dataset_dir = root / args.dataset_dir
    models_dir = root / args.models_dir
    main_dir = root / args.main_dir
    horizon_steps = max(1, args.horizon_minutes // 5)

    np.random.seed(42)
    tf.random.set_seed(42)

    csv_path = ensure_dataset(dataset_dir)
    data = prepare_data(csv_path, horizon_steps, args.validation_ratio)
    model = train_model(data, args.epochs, args.batch_size)

    models_dir.mkdir(parents=True, exist_ok=True)
    saved_model_dir = models_dir / "soil_moisture_saved_model"
    if hasattr(model, "export"):
        model.export(saved_model_dir)
    else:
        tf.saved_model.save(model, saved_model_dir)

    float_model = convert_float_model(model)
    int8_model = convert_quantized_model(model, data.x_train)

    float_path = models_dir / "soil_moisture_level_float.tflite"
    int8_path = models_dir / "soil_moisture_level_int8.tflite"
    float_path.write_bytes(float_model)
    int8_path.write_bytes(int8_model)

    float_pred = run_tflite(float_model, data.x_val[:1200])
    int8_pred = run_tflite(int8_model, data.x_val[:1200])
    y_eval = data.y_val[:1200]
    metrics = {
        "float": regression_metrics(y_eval, float_pred),
        "int8": regression_metrics(y_eval, int8_pred),
    }

    write_model_data(int8_model, main_dir / "soil_model_data.cc")
    write_metadata(data, metrics, main_dir / "soil_model_metadata.h", args.horizon_minutes)

    print("Dataset:", csv_path)
    print("Samples:", data.sample_count)
    print("Float model:", float_path)
    print("Int8 model:", int8_path)
    print("Float metrics:", metrics["float"])
    print("Int8 metrics:", metrics["int8"])
    print("Firmware model:", main_dir / "soil_model_data.cc")
    print("Firmware metadata:", main_dir / "soil_model_metadata.h")


if __name__ == "__main__":
    os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
    main()
