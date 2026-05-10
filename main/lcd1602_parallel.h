#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gpio_num_t pino_rs;
    gpio_num_t pino_e;
    gpio_num_t pinos_dados[4];
} lcd1602_parallel_config_t;

typedef struct {
    lcd1602_parallel_config_t configuracao;
} lcd1602_parallel_t;

esp_err_t lcd1602_parallel_iniciar(lcd1602_parallel_t *lcd,
                                   const lcd1602_parallel_config_t *configuracao);
esp_err_t lcd1602_parallel_escrever_linha(lcd1602_parallel_t *lcd,
                                          uint8_t linha,
                                          const char *texto);

#ifdef __cplusplus
}
#endif
