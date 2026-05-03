#pragma once

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"

typedef struct {
    int16_t temperatura_decimos;
    uint16_t umidade_decimos;
} dht22_leitura_t;

esp_err_t dht22_iniciar(gpio_num_t pino_dados);
esp_err_t dht22_ler(gpio_num_t pino_dados, dht22_leitura_t *leitura);

