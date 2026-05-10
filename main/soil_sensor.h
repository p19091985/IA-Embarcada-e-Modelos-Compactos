#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    adc_oneshot_unit_handle_t unidade;
    adc_channel_t canal;
} soil_sensor_t;

esp_err_t soil_sensor_iniciar(soil_sensor_t *sensor, adc_unit_t unidade, adc_channel_t canal);
esp_err_t soil_sensor_ler_bruto(soil_sensor_t *sensor, int *raw);

#ifdef __cplusplus
}
#endif

