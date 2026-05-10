#include "lcd1602_parallel.h"

#include <stdbool.h>
#include <stdio.h>

#include "esp_check.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG_LCD_PARALLEL = "lcd1602_parallel";

static esp_err_t lcd_parallel_enviar_nibble(lcd1602_parallel_t *lcd, uint8_t nibble);
static esp_err_t lcd_parallel_pulsar_enable(lcd1602_parallel_t *lcd);
static esp_err_t lcd_parallel_enviar(lcd1602_parallel_t *lcd, uint8_t valor, bool modo_dados);
static esp_err_t lcd_parallel_comando(lcd1602_parallel_t *lcd, uint8_t comando);
static esp_err_t lcd_parallel_definir_caractere(lcd1602_parallel_t *lcd, uint8_t indice, const uint8_t bitmap[8]);
static esp_err_t lcd_parallel_escrever_caractere(lcd1602_parallel_t *lcd, char caractere);
static esp_err_t lcd_parallel_posicionar_cursor(lcd1602_parallel_t *lcd, uint8_t coluna, uint8_t linha);

esp_err_t lcd1602_parallel_iniciar(lcd1602_parallel_t *lcd,
                                   const lcd1602_parallel_config_t *configuracao)
{
    lcd->configuracao = *configuracao;

    gpio_config_t config = {
        .pin_bit_mask = (1ULL << configuracao->pino_rs) | (1ULL << configuracao->pino_e),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    for (int i = 0; i < 4; i++) {
        config.pin_bit_mask |= (1ULL << configuracao->pinos_dados[i]);
    }

    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG_LCD_PARALLEL, "falha ao configurar pinos");
    ESP_RETURN_ON_ERROR(gpio_set_level(configuracao->pino_rs, 0), TAG_LCD_PARALLEL, "falha RS");
    ESP_RETURN_ON_ERROR(gpio_set_level(configuracao->pino_e, 0), TAG_LCD_PARALLEL, "falha E");

    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_RETURN_ON_ERROR(lcd_parallel_enviar_nibble(lcd, 0x03), TAG_LCD_PARALLEL, "falha init 1");
    vTaskDelay(pdMS_TO_TICKS(5));
    ESP_RETURN_ON_ERROR(lcd_parallel_enviar_nibble(lcd, 0x03), TAG_LCD_PARALLEL, "falha init 2");
    esp_rom_delay_us(150);
    ESP_RETURN_ON_ERROR(lcd_parallel_enviar_nibble(lcd, 0x03), TAG_LCD_PARALLEL, "falha init 3");
    ESP_RETURN_ON_ERROR(lcd_parallel_enviar_nibble(lcd, 0x02), TAG_LCD_PARALLEL, "falha modo 4 bits");

    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x28), TAG_LCD_PARALLEL, "falha function set");
    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x08), TAG_LCD_PARALLEL, "falha display off");
    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x01), TAG_LCD_PARALLEL, "falha limpar");
    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x06), TAG_LCD_PARALLEL, "falha entry mode");
    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x0C), TAG_LCD_PARALLEL, "falha display on");

    const uint8_t c_cedilha[8] = {
        0x0E,
        0x11,
        0x10,
        0x10,
        0x11,
        0x0E,
        0x04,
        0x0C,
    };
    const uint8_t a_til[8] = {
        0x0A,
        0x04,
        0x0E,
        0x11,
        0x1F,
        0x11,
        0x11,
        0x00,
    };
    const uint8_t i_agudo[8] = {
        0x02,
        0x04,
        0x0E,
        0x04,
        0x04,
        0x04,
        0x0E,
        0x00,
    };

    ESP_RETURN_ON_ERROR(lcd_parallel_definir_caractere(lcd, 1, c_cedilha), TAG_LCD_PARALLEL, "falha caractere Ç");
    ESP_RETURN_ON_ERROR(lcd_parallel_definir_caractere(lcd, 2, a_til), TAG_LCD_PARALLEL, "falha caractere Ã");
    ESP_RETURN_ON_ERROR(lcd_parallel_definir_caractere(lcd, 3, i_agudo), TAG_LCD_PARALLEL, "falha caractere Í");

    return ESP_OK;
}

esp_err_t lcd1602_parallel_escrever_linha(lcd1602_parallel_t *lcd,
                                          uint8_t linha,
                                          const char *texto)
{
    char texto_com_16_colunas[17];

    snprintf(texto_com_16_colunas, sizeof(texto_com_16_colunas), "%-16.16s", texto);
    ESP_RETURN_ON_ERROR(lcd_parallel_posicionar_cursor(lcd, 0, linha), TAG_LCD_PARALLEL, "falha cursor");

    for (int coluna = 0; coluna < 16; coluna++) {
        ESP_RETURN_ON_ERROR(lcd_parallel_escrever_caractere(lcd, texto_com_16_colunas[coluna]), TAG_LCD_PARALLEL, "falha caractere");
    }

    return ESP_OK;
}

static esp_err_t lcd_parallel_enviar_nibble(lcd1602_parallel_t *lcd, uint8_t nibble)
{
    for (int bit = 0; bit < 4; bit++) {
        ESP_RETURN_ON_ERROR(gpio_set_level(lcd->configuracao.pinos_dados[bit], (nibble >> bit) & 0x01),
                            TAG_LCD_PARALLEL,
                            "falha dado");
    }

    return lcd_parallel_pulsar_enable(lcd);
}

static esp_err_t lcd_parallel_pulsar_enable(lcd1602_parallel_t *lcd)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(lcd->configuracao.pino_e, 1), TAG_LCD_PARALLEL, "falha enable alto");
    esp_rom_delay_us(1);
    ESP_RETURN_ON_ERROR(gpio_set_level(lcd->configuracao.pino_e, 0), TAG_LCD_PARALLEL, "falha enable baixo");
    esp_rom_delay_us(50);
    return ESP_OK;
}

static esp_err_t lcd_parallel_enviar(lcd1602_parallel_t *lcd, uint8_t valor, bool modo_dados)
{
    ESP_RETURN_ON_ERROR(gpio_set_level(lcd->configuracao.pino_rs, modo_dados ? 1 : 0), TAG_LCD_PARALLEL, "falha RS modo");
    ESP_RETURN_ON_ERROR(lcd_parallel_enviar_nibble(lcd, valor >> 4), TAG_LCD_PARALLEL, "falha nibble alto");
    return lcd_parallel_enviar_nibble(lcd, valor & 0x0F);
}

static esp_err_t lcd_parallel_comando(lcd1602_parallel_t *lcd, uint8_t comando)
{
    esp_err_t resultado = lcd_parallel_enviar(lcd, comando, false);

    if (comando == 0x01 || comando == 0x02) {
        vTaskDelay(pdMS_TO_TICKS(2));
    }

    return resultado;
}

static esp_err_t lcd_parallel_definir_caractere(lcd1602_parallel_t *lcd, uint8_t indice, const uint8_t bitmap[8])
{
    indice &= 0x07;
    ESP_RETURN_ON_ERROR(lcd_parallel_comando(lcd, 0x40 | (indice << 3)), TAG_LCD_PARALLEL, "falha CGRAM");

    for (int linha = 0; linha < 8; linha++) {
        ESP_RETURN_ON_ERROR(lcd_parallel_enviar(lcd, bitmap[linha], true), TAG_LCD_PARALLEL, "falha bitmap");
    }

    return ESP_OK;
}

static esp_err_t lcd_parallel_escrever_caractere(lcd1602_parallel_t *lcd, char caractere)
{
    return lcd_parallel_enviar(lcd, (uint8_t)caractere, true);
}

static esp_err_t lcd_parallel_posicionar_cursor(lcd1602_parallel_t *lcd, uint8_t coluna, uint8_t linha)
{
    const uint8_t inicio_linhas[] = {0x00, 0x40};

    if (linha > 1) {
        linha = 1;
    }
    if (coluna > 15) {
        coluna = 15;
    }

    return lcd_parallel_comando(lcd, 0x80 | (uint8_t)(coluna + inicio_linhas[linha]));
}
