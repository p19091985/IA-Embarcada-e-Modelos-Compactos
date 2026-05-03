#include "dht22.h"

#include <stdbool.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#define DHT22_TEMPO_SINAL_INICIAL_US 1100
#define DHT22_TEMPO_RESPOSTA_US 100
#define DHT22_TEMPO_INICIO_BIT_US 80
#define DHT22_TEMPO_LEITURA_BIT_US 120
#define DHT22_LIMITE_BIT_UM_US 50
#define DHT22_TOTAL_BITS 40

static const char *TAG_DHT22 = "dht22";

static void dht22_configurar_entrada(gpio_num_t pino_dados);
static void dht22_configurar_saida_baixa(gpio_num_t pino_dados);
static esp_err_t dht22_esperar_mudanca(gpio_num_t pino_dados, int nivel_atual, uint32_t tempo_limite_us, uint32_t *duracao_us);

esp_err_t dht22_iniciar(gpio_num_t pino_dados)
{
    dht22_configurar_entrada(pino_dados);
    return ESP_OK;
}

// segue o protocolo do datasheet do DHT22 pra pegar os 40 bits
esp_err_t dht22_ler(gpio_num_t pino_dados, dht22_leitura_t *leitura)
{
    uint8_t bytes_recebidos[5] = {0};

    dht22_configurar_saida_baixa(pino_dados);
    esp_rom_delay_us(DHT22_TEMPO_SINAL_INICIAL_US);
    gpio_set_level(pino_dados, 1);
    esp_rom_delay_us(40);
    dht22_configurar_entrada(pino_dados);

    ESP_RETURN_ON_ERROR(dht22_esperar_mudanca(pino_dados, 1, DHT22_TEMPO_RESPOSTA_US, NULL), TAG_DHT22, "sensor nao respondeu");
    ESP_RETURN_ON_ERROR(dht22_esperar_mudanca(pino_dados, 0, DHT22_TEMPO_RESPOSTA_US, NULL), TAG_DHT22, "resposta baixa falhou");
    ESP_RETURN_ON_ERROR(dht22_esperar_mudanca(pino_dados, 1, DHT22_TEMPO_RESPOSTA_US, NULL), TAG_DHT22, "resposta alta falhou");

    for (int bit_atual = 0; bit_atual < DHT22_TOTAL_BITS; bit_atual++) {
        uint32_t tempo_em_nivel_alto_us = 0;
        int indice_byte = bit_atual / 8;

        ESP_RETURN_ON_ERROR(dht22_esperar_mudanca(pino_dados, 0, DHT22_TEMPO_INICIO_BIT_US, NULL), TAG_DHT22, "inicio do bit falhou");
        ESP_RETURN_ON_ERROR(dht22_esperar_mudanca(pino_dados, 1, DHT22_TEMPO_LEITURA_BIT_US, &tempo_em_nivel_alto_us), TAG_DHT22, "leitura do bit falhou");

        bytes_recebidos[indice_byte] <<= 1;
        if (tempo_em_nivel_alto_us > DHT22_LIMITE_BIT_UM_US) {
            bytes_recebidos[indice_byte] |= 1;
        }
    }

    uint8_t soma_calculada = (uint8_t)(bytes_recebidos[0] + bytes_recebidos[1] + bytes_recebidos[2] + bytes_recebidos[3]);
    if (soma_calculada != bytes_recebidos[4]) {
        return ESP_ERR_INVALID_CRC;
    }

    uint16_t umidade = ((uint16_t)bytes_recebidos[0] << 8) | bytes_recebidos[1];
    uint16_t temperatura = ((uint16_t)bytes_recebidos[2] << 8) | bytes_recebidos[3];
    bool temperatura_negativa = (temperatura & 0x8000) != 0;

    temperatura &= 0x7FFF;
    leitura->umidade_decimos = umidade;
    leitura->temperatura_decimos = temperatura_negativa ? -(int16_t)temperatura : (int16_t)temperatura;

    return ESP_OK;
}

static void dht22_configurar_entrada(gpio_num_t pino_dados)
{
    gpio_set_direction(pino_dados, GPIO_MODE_INPUT);
    gpio_set_pull_mode(pino_dados, GPIO_PULLUP_ONLY);
}

static void dht22_configurar_saida_baixa(gpio_num_t pino_dados)
{
    gpio_set_direction(pino_dados, GPIO_MODE_OUTPUT_OD);
    gpio_set_pull_mode(pino_dados, GPIO_PULLUP_ONLY);
    gpio_set_level(pino_dados, 0);
}

static esp_err_t dht22_esperar_mudanca(gpio_num_t pino_dados, int nivel_atual, uint32_t tempo_limite_us, uint32_t *duracao_us)
{
    int64_t tempo_inicial_us = esp_timer_get_time();

    while (gpio_get_level(pino_dados) == nivel_atual) {
        int64_t tempo_passado_us = esp_timer_get_time() - tempo_inicial_us;
        if (tempo_passado_us > tempo_limite_us) {
            return ESP_ERR_TIMEOUT;
        }
    }

    if (duracao_us != NULL) {
        *duracao_us = (uint32_t)(esp_timer_get_time() - tempo_inicial_us);
    }

    return ESP_OK;
}

