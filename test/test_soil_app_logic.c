#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "soil_app_logic.h"

static void test_adc_to_percent(void)
{
    assert(fabsf(soil_adc_to_percent(0, 0, 3504) - 0.0f) < 0.001f);
    assert(fabsf(soil_adc_to_percent(1752, 0, 3504) - 50.0f) < 0.001f);
    assert(fabsf(soil_adc_to_percent(3504, 0, 3504) - 100.0f) < 0.001f);
    assert(fabsf(soil_adc_to_percent(5000, 0, 3504) - 100.0f) < 0.001f);
    assert(fabsf(soil_adc_to_percent(-10, 0, 3504) - 0.0f) < 0.001f);
}

static void test_percent_to_level(void)
{
    assert(soil_percent_to_level(-10.0f) == 0);
    assert(soil_percent_to_level(0.0f) == 0);
    assert(soil_percent_to_level(50.0f) == 5);
    assert(soil_percent_to_level(100.0f) == 9);
    assert(soil_percent_to_level(200.0f) == 9);
}

static void test_prediction_to_level(void)
{
    assert(soil_prediction_to_level(-1.0f) == 0);
    assert(soil_prediction_to_level(0.49f) == 0);
    assert(soil_prediction_to_level(0.50f) == 1);
    assert(soil_prediction_to_level(4.49f) == 4);
    assert(soil_prediction_to_level(4.50f) == 5);
    assert(soil_prediction_to_level(20.0f) == 9);
}

static void test_feature_defaults_and_normalization(void)
{
    const float defaults[SOIL_MODEL_INPUT_COUNT] = {45.0f, 0.0f, 25.0f, 55.0f, 101.0f, 2.0f, 8.0f, 180.0f, 1.0f};
    const float min_values[SOIL_MODEL_INPUT_COUNT] = {0.0f, 0.0f, 10.0f, 0.0f, 100.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const float max_values[SOIL_MODEL_INPUT_COUNT] = {100.0f, 1.0f, 40.0f, 100.0f, 102.0f, 20.0f, 40.0f, 360.0f, 4.0f};
    float features[SOIL_MODEL_INPUT_COUNT] = {0};
    float normalized[SOIL_MODEL_INPUT_COUNT] = {0};

    soil_features_from_defaults(defaults, 70.0f, true, features);

    assert(fabsf(features[0] - 70.0f) < 0.001f);
    assert(fabsf(features[1] - 1.0f) < 0.001f);
    assert(fabsf(features[2] - 25.0f) < 0.001f);

    soil_normalize_features(features, min_values, max_values, normalized);

    assert(fabsf(normalized[0] - 0.70f) < 0.001f);
    assert(fabsf(normalized[1] - 1.00f) < 0.001f);
    assert(fabsf(normalized[2] - 0.50f) < 0.001f);
    assert(fabsf(normalized[8] - 0.00f) < 0.001f);
}

static void test_lcd_format(void)
{
    char line0[17] = {0};
    char line1[17] = {0};

    soil_format_lcd_lines(1234, 56.4f, 5, true, line0, line1);

    assert(strlen(line0) <= 16);
    assert(strlen(line1) <= 16);
    assert(strcmp(line0, SOIL_LCD_WORD_MEDICAO " IA N:5") == 0);
    assert(strstr(line1, "56%") != NULL);
    assert(strstr(line1, "1234") != NULL);

    soil_format_linear_lcd_lines(1234, 56.4f, 5, line0, line1);

    assert(strlen(line0) <= 16);
    assert(strlen(line1) <= 16);
    assert(strcmp(line0, SOIL_LCD_WORD_MEDICAO " LINEAR") == 0);
    assert(strstr(line1, "N:5") != NULL);
    assert(strstr(line1, "1234") != NULL);
}

int main(void)
{
    test_adc_to_percent();
    test_percent_to_level();
    test_prediction_to_level();
    test_feature_defaults_and_normalization();
    test_lcd_format();
    puts("test_soil_app_logic: ok");
    return 0;
}
