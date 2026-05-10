#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SOIL_MODEL_INPUT_COUNT 9
#define SOIL_SENSOR_RAW_MIN 0
#define SOIL_SENSOR_RAW_MAX 3504
#define SOIL_LCD_CHAR_C_CEDILHA "\001"
#define SOIL_LCD_CHAR_A_TIL "\002"
#define SOIL_LCD_CHAR_I_AGUDO "\003"
#define SOIL_LCD_WORD_MEDICAO "MEDI" SOIL_LCD_CHAR_C_CEDILHA SOIL_LCD_CHAR_A_TIL "O"
#define SOIL_LCD_WORD_DIGITO "D" SOIL_LCD_CHAR_I_AGUDO "GITO"

typedef struct {
    float sensor_humidity;
    float irrigation;
    float air_temperature;
    float air_humidity;
    float pressure;
    float wind_speed;
    float wind_gust;
    float wind_direction;
    float field_id;
} soil_features_t;

float soil_clampf(float valor, float minimo, float maximo);
float soil_adc_to_percent(int raw, int raw_min, int raw_max);
int soil_percent_to_level(float percent);
int soil_prediction_to_level(float prediction);

void soil_features_from_defaults(const float defaults[SOIL_MODEL_INPUT_COUNT],
                                 float sensor_percent,
                                 bool irrigation_on,
                                 float features[SOIL_MODEL_INPUT_COUNT]);

void soil_normalize_features(const float features[SOIL_MODEL_INPUT_COUNT],
                             const float min_values[SOIL_MODEL_INPUT_COUNT],
                             const float max_values[SOIL_MODEL_INPUT_COUNT],
                             float normalized[SOIL_MODEL_INPUT_COUNT]);

void soil_format_lcd_lines(int raw_adc,
                           float sensor_percent,
                           int predicted_level,
                           bool model_ready,
                           char line0[17],
                           char line1[17]);

void soil_format_linear_lcd_lines(int raw_adc,
                                  float sensor_percent,
                                  int linear_level,
                                  char line0[17],
                                  char line1[17]);

#ifdef __cplusplus
}
#endif
