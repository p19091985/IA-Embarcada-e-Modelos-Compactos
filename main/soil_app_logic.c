#include "soil_app_logic.h"

#include <stdio.h>

float soil_clampf(float valor, float minimo, float maximo)
{
    if (valor < minimo) {
        return minimo;
    }
    if (valor > maximo) {
        return maximo;
    }
    return valor;
}

float soil_adc_to_percent(int raw, int raw_min, int raw_max)
{
    if (raw_max <= raw_min) {
        return 0.0f;
    }

    float percent = ((float)(raw - raw_min) * 100.0f) / (float)(raw_max - raw_min);
    return soil_clampf(percent, 0.0f, 100.0f);
}

int soil_percent_to_level(float percent)
{
    float clamped = soil_clampf(percent, 0.0f, 100.0f);
    int level = (int)((clamped / 100.0f) * 9.0f + 0.5f);

    if (level < 0) {
        return 0;
    }
    if (level > 9) {
        return 9;
    }
    return level;
}

int soil_prediction_to_level(float prediction)
{
    float clamped = soil_clampf(prediction, 0.0f, 9.0f);
    int level = (int)(clamped + 0.5f);

    if (level < 0) {
        return 0;
    }
    if (level > 9) {
        return 9;
    }
    return level;
}

void soil_features_from_defaults(const float defaults[SOIL_MODEL_INPUT_COUNT],
                                 float sensor_percent,
                                 bool irrigation_on,
                                 float features[SOIL_MODEL_INPUT_COUNT])
{
    for (size_t i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        features[i] = defaults[i];
    }

    features[0] = soil_clampf(sensor_percent, 0.0f, 100.0f);
    features[1] = irrigation_on ? 1.0f : 0.0f;
}

void soil_normalize_features(const float features[SOIL_MODEL_INPUT_COUNT],
                             const float min_values[SOIL_MODEL_INPUT_COUNT],
                             const float max_values[SOIL_MODEL_INPUT_COUNT],
                             float normalized[SOIL_MODEL_INPUT_COUNT])
{
    for (size_t i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        float span = max_values[i] - min_values[i];

        if (span <= 0.000001f) {
            normalized[i] = 0.0f;
            continue;
        }

        normalized[i] = soil_clampf((features[i] - min_values[i]) / span, 0.0f, 1.0f);
    }
}

void soil_format_lcd_lines(int raw_adc,
                           float sensor_percent,
                           int predicted_level,
                           bool model_ready,
                           char line0[17],
                           char line1[17])
{
    if (model_ready) {
        snprintf(line0, 17, SOIL_LCD_WORD_MEDICAO " IA N:%d", predicted_level);
        snprintf(line1, 17, "%3.0f%% ADC%4d", soil_clampf(sensor_percent, 0.0f, 100.0f), raw_adc);
        return;
    }

    snprintf(line0, 17, SOIL_LCD_WORD_MEDICAO " IA");
    snprintf(line1, 17, "MODELO OFF %4d", raw_adc);
}

void soil_format_linear_lcd_lines(int raw_adc,
                                  float sensor_percent,
                                  int linear_level,
                                  char line0[17],
                                  char line1[17])
{
    snprintf(line0, 17, SOIL_LCD_WORD_MEDICAO " LINEAR");
    snprintf(line1, 17, "N:%d %3.0f%% ADC%4d", linear_level, soil_clampf(sensor_percent, 0.0f, 100.0f), raw_adc);
}
