#!/usr/bin/env python3

import csv
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
DATASET = ROOT / "datasets" / "Wazihub_Soil_Moisture_Prediction" / "Dataset" / "Train.csv"
MODEL_CC = ROOT / "main" / "soil_model_data.cc"
METADATA_H = ROOT / "main" / "soil_model_metadata.h"
MODEL_DIR = ROOT / "main" / "models"


class TrainingDatasetTest(unittest.TestCase):
    def test_dataset_was_downloaded(self):
        self.assertTrue(DATASET.exists(), f"dataset ausente: {DATASET}")

    def test_expected_columns_exist(self):
        with DATASET.open(newline="") as handle:
            reader = csv.DictReader(handle)
            columns = set(reader.fieldnames or [])

        expected = {
            "timestamp",
            "Soil humidity 1",
            "Irrigation field 1",
            "Soil humidity 2",
            "Irrigation field 2",
            "Soil humidity 3",
            "Irrigation field 3",
            "Soil humidity 4",
            "Irrigation field 4",
            "Air temperature (C)",
            "Air humidity (%)",
            "Pressure (KPa)",
            "Wind speed (Km/h)",
            "Wind gust (Km/h)",
            "Wind direction (Deg)",
        }
        self.assertTrue(expected.issubset(columns))

    def test_firmware_model_files_exist(self):
        self.assertTrue(MODEL_CC.exists())
        self.assertTrue(METADATA_H.exists())
        self.assertIn("g_soil_moisture_model", MODEL_CC.read_text(encoding="utf-8"))
        self.assertIn("kSoilFeatureDefaults", METADATA_H.read_text(encoding="utf-8"))

    def test_training_outputs_are_inside_main(self):
        self.assertTrue((MODEL_DIR / "soil_moisture_level_float.tflite").exists())
        self.assertTrue((MODEL_DIR / "soil_moisture_level_int8.tflite").exists())
        self.assertTrue((MODEL_DIR / "soil_moisture_saved_model" / "saved_model.pb").exists())


if __name__ == "__main__":
    unittest.main()
