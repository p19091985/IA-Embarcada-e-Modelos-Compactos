#include "soil_sensor.h"

#include "esp_check.h"

static const char *TAG_SOIL_SENSOR = "soil_sensor";

esp_err_t soil_sensor_iniciar(soil_sensor_t *sensor, adc_unit_t unidade, adc_channel_t canal)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = unidade,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    sensor->canal = canal;

    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&init_config, &sensor->unidade), TAG_SOIL_SENSOR, "falha ao iniciar ADC");
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(sensor->unidade, canal, &channel_config), TAG_SOIL_SENSOR, "falha ao configurar canal ADC");

    return ESP_OK;
}

esp_err_t soil_sensor_ler_bruto(soil_sensor_t *sensor, int *raw)
{
    return adc_oneshot_read(sensor->unidade, sensor->canal, raw);
}

