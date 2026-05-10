#include "seven_segment_display.h"

#include "esp_check.h"

static const char *TAG_SEVEN_SEG = "seven_segment";

esp_err_t seven_segment_display_iniciar(const seven_segment_display_t *display)
{
    gpio_config_t config = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (int i = 0; i < SEVEN_SEGMENT_COUNT; i++) {
        config.pin_bit_mask |= (1ULL << display->pinos[i]);
    }

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG_SEVEN_SEG, "falha ao configurar pinos do 7 segmentos");
    return seven_segment_display_exibir(display, 0);
}

esp_err_t seven_segment_display_exibir(const seven_segment_display_t *display, int digit)
{
    uint8_t segmentos[SEVEN_SEGMENT_COUNT] = {0};

    seven_segment_logic_encode_digit(digit, display->ativo_alto, segmentos);

    for (int i = 0; i < SEVEN_SEGMENT_COUNT; i++) {
        ESP_RETURN_ON_ERROR(gpio_set_level(display->pinos[i], segmentos[i]), TAG_SEVEN_SEG, "falha ao escrever segmento");
    }

    return ESP_OK;
}
