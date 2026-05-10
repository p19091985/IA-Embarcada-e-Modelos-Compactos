#pragma once

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_err.h"

#include "seven_segment_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pinos[SEVEN_SEGMENT_COUNT];
    bool ativo_alto;
} seven_segment_display_t;

esp_err_t seven_segment_display_iniciar(const seven_segment_display_t *display);
esp_err_t seven_segment_display_exibir(const seven_segment_display_t *display, int digit);

#ifdef __cplusplus
}
#endif
