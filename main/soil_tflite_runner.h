#pragma once

#include <stdbool.h>

#include "esp_err.h"

#include "soil_app_logic.h"

esp_err_t soil_tflite_iniciar(void);
bool soil_tflite_pronto(void);
esp_err_t soil_tflite_prever_nivel(const float normalized_features[SOIL_MODEL_INPUT_COUNT],
                                   float *predicted_level);

