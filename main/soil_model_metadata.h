#pragma once

#include "soil_app_logic.h"

#define SOIL_MODEL_NAME "soil_moisture_level_int8"
#define SOIL_MODEL_HORIZON_MINUTES 60
#define SOIL_MODEL_SAMPLE_COUNT 71786
#define SOIL_MODEL_FLOAT_MAE 0.068021014f
#define SOIL_MODEL_INT8_MAE 0.068855487f

static const float kSoilFeatureMin[SOIL_MODEL_INPUT_COUNT] = {
    0.0f, 0.0f, 11.22f, 0.58999997f, 100.5f, 0.0f, 0.0f, 0.0f, 1.0f,
};

static const float kSoilFeatureMax[SOIL_MODEL_INPUT_COUNT] = {
    88.0f, 1.0f, 45.560001f, 96.0f, 101.86f, 31.360001f, 133.33f, 337.5f, 4.0f,
};

static const float kSoilFeatureDefaults[SOIL_MODEL_INPUT_COUNT] = {
    26.26f, 0.0f, 22.290001f, 56.599998f, 101.15f, 9.2799997f, 36.560001f, 22.5f, 3.0f,
};

static const char *const kSoilFeatureNames[SOIL_MODEL_INPUT_COUNT] = {
    "sensor_humidity",
    "irrigation",
    "air_temperature",
    "air_humidity",
    "pressure",
    "wind_speed",
    "wind_gust",
    "wind_direction",
    "field_id",
};
