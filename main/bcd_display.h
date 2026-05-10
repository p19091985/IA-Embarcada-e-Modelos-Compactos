#pragma once

#include "driver/gpio.h"
#include "esp_err.h"

#include "bcd_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pinos[BCD_LOGIC_BIT_COUNT];
} bcd_display_t;

esp_err_t bcd_display_iniciar(const bcd_display_t *display);
esp_err_t bcd_display_exibir(const bcd_display_t *display, int digit);

#ifdef __cplusplus
}
#endif

