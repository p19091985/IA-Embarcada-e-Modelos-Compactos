#include "bcd_display.h"

#include "esp_check.h"

static const char *TAG_BCD = "bcd_display";

esp_err_t bcd_display_iniciar(const bcd_display_t *display)
{
    gpio_config_t config = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (int i = 0; i < BCD_LOGIC_BIT_COUNT; i++) {
        config.pin_bit_mask |= (1ULL << display->pinos[i]);
    }

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG_BCD, "falha ao configurar pinos BCD");
    return bcd_display_exibir(display, 0);
}

esp_err_t bcd_display_exibir(const bcd_display_t *display, int digit)
{
    uint8_t bits[BCD_LOGIC_BIT_COUNT] = {0};

    bcd_logic_encode_digit(digit, bits);

    for (int i = 0; i < BCD_LOGIC_BIT_COUNT; i++) {
        ESP_RETURN_ON_ERROR(gpio_set_level(display->pinos[i], bits[i]), TAG_BCD, "falha ao escrever pino BCD");
    }

    return ESP_OK;
}

