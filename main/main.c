#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dht22.h"
#include "lcd1602_i2c.h"

#define PORTA_I2C I2C_NUM_0
#define PINO_SDA_LCD GPIO_NUM_1
#define PINO_SCL_LCD GPIO_NUM_2
#define PINO_DADOS_DHT22 GPIO_NUM_4

#define ENDERECO_LCD 0x27
#define FREQUENCIA_I2C_HZ 100000
#define INTERVALO_LEITURA_MS 2000

static const char *TAG = "codigo_principal";

// declaracoes das funcoes auxiliares
static void formatar_decimal(char *texto, size_t tamanho, int valor_decimos);
static esp_err_t mostrar_leitura_no_lcd(lcd1602_t *lcd, const char *temperatura, const char *umidade);
static void mostrar_leitura_no_serial(const char *temperatura, const char *umidade);

void app_main(void)
{
    // configura o lcd e o sensor antes de entrar no loop
    lcd1602_t lcd = {0};
    lcd1602_config_t configuracao_lcd = {
        .porta_i2c = PORTA_I2C,
        .pino_sda = PINO_SDA_LCD,
        .pino_scl = PINO_SCL_LCD,
        .endereco = ENDERECO_LCD,
        .frequencia_hz = FREQUENCIA_I2C_HZ,
    };

    ESP_LOGI(TAG, "Iniciando aplicacao ESP-IDF com Wokwi");
    ESP_LOGI(TAG, "LCD1602: SDA=GPIO%d, SCL=GPIO%d", PINO_SDA_LCD, PINO_SCL_LCD);
    ESP_LOGI(TAG, "DHT22: DATA=GPIO%d", PINO_DADOS_DHT22);

    ESP_ERROR_CHECK(lcd1602_iniciar(&lcd, &configuracao_lcd));
    ESP_ERROR_CHECK(dht22_iniciar(PINO_DADOS_DHT22));

    ESP_ERROR_CHECK(lcd1602_escrever_linha(&lcd, 0, "DHT22 + LCD"));
    ESP_ERROR_CHECK(lcd1602_escrever_linha(&lcd, 1, "Aguardando..."));

    // loop principal - le o sensor e atualiza lcd + serial
    while (true) {
        dht22_leitura_t leitura = {0};
        esp_err_t resultado = dht22_ler(PINO_DADOS_DHT22, &leitura);

        if (resultado == ESP_OK) {
            char temperatura[8];
            char umidade[8];

            formatar_decimal(temperatura, sizeof(temperatura), leitura.temperatura_decimos);
            formatar_decimal(umidade, sizeof(umidade), leitura.umidade_decimos);

            mostrar_leitura_no_serial(temperatura, umidade);
            ESP_ERROR_CHECK(mostrar_leitura_no_lcd(&lcd, temperatura, umidade));
        } else {
            ESP_LOGE(TAG, "Falha ao ler DHT22: %s", esp_err_to_name(resultado));
            ESP_ERROR_CHECK(lcd1602_escrever_linha(&lcd, 0, "Falha no DHT22"));
            ESP_ERROR_CHECK(lcd1602_escrever_linha(&lcd, 1, "Tentando..."));
        }

        vTaskDelay(pdMS_TO_TICKS(INTERVALO_LEITURA_MS));
    }
}

// converte valor em decimos (ex: 245 -> "24.5")
static void formatar_decimal(char *texto, size_t tamanho, int valor_decimos)
{
    int valor_absoluto = abs(valor_decimos);
    const char *sinal = valor_decimos < 0 ? "-" : "";

    snprintf(texto, tamanho, "%s%d.%d", sinal, valor_absoluto / 10, valor_absoluto % 10);
}

static esp_err_t mostrar_leitura_no_lcd(lcd1602_t *lcd, const char *temperatura, const char *umidade)
{
    char linha_temperatura[17];
    char linha_umidade[17];

    snprintf(linha_temperatura, sizeof(linha_temperatura), "Temp: %s C", temperatura);
    snprintf(linha_umidade, sizeof(linha_umidade), "Umid: %s %%", umidade);

    ESP_RETURN_ON_ERROR(lcd1602_escrever_linha(lcd, 0, linha_temperatura), TAG, "falha ao mostrar temperatura");
    ESP_RETURN_ON_ERROR(lcd1602_escrever_linha(lcd, 1, linha_umidade), TAG, "falha ao mostrar umidade");

    return ESP_OK;
}

static void mostrar_leitura_no_serial(const char *temperatura, const char *umidade)
{
    ESP_LOGI(TAG, "Temperatura: %s C | Umidade: %s %%", temperatura, umidade);
}

