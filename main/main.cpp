/*
 * main.cpp — entry point do sistema de monitoramento de umidade do solo.
 *
 * Leitura contínua do sensor ADC, inferência TFLite Micro (modelo int8
 * treinado com o dataset Wazihub) e exibição dupla: a medição por IA
 * aparece no LCD I2C e no 7-seg via CD4511, enquanto a medição linear
 * aparece no LCD paralelo e no 7-seg direto. LEDs azul e dourado indicam
 * necessidade de irrigação conforme cada critério.
 */

#include <stdbool.h>
#include <stdio.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bcd_display.h"
#include "lcd1602_i2c.h"
#include "lcd1602_parallel.h"
#include "seven_segment_display.h"
#include "soil_app_logic.h"
#include "soil_model_metadata.h"
#include "soil_sensor.h"
#include "soil_tflite_runner.h"

// LCD I2C
#define PORTA_I2C_LCD_PRINCIPAL I2C_NUM_0
#define PINO_SDA_LCD_PRINCIPAL GPIO_NUM_1
#define PINO_SCL_LCD_PRINCIPAL GPIO_NUM_2

// LCD Paralelo
#define PINO_LCD_PAR_RS GPIO_NUM_18
#define PINO_LCD_PAR_E  GPIO_NUM_19
#define PINO_LCD_PAR_D4 GPIO_NUM_20
#define PINO_LCD_PAR_D5 GPIO_NUM_21
#define PINO_LCD_PAR_D6 GPIO_NUM_35
#define PINO_LCD_PAR_D7 GPIO_NUM_36

// Sensor de umidade (slider no Wokwi, ADC1_CH3 = GPIO4)
#define PINO_SENSOR_UMIDADE    GPIO_NUM_4
#define UNIDADE_SENSOR_UMIDADE ADC_UNIT_1
#define CANAL_SENSOR_UMIDADE   ADC_CHANNEL_3

// CD4511 BCD -> digit1
#define PINO_BCD_A GPIO_NUM_5
#define PINO_BCD_B GPIO_NUM_6
#define PINO_BCD_C GPIO_NUM_7
#define PINO_BCD_D GPIO_NUM_8

// 7-segmentos direto -> digit2
#define PINO_SEG_A  GPIO_NUM_9
#define PINO_SEG_B  GPIO_NUM_10
#define PINO_SEG_C  GPIO_NUM_11
#define PINO_SEG_D  GPIO_NUM_12
#define PINO_SEG_E  GPIO_NUM_13
#define PINO_SEG_F  GPIO_NUM_14
#define PINO_SEG_G  GPIO_NUM_15
#define PINO_SEG_DP GPIO_NUM_37

// LEDs de irrigação por tipo de medição
#define PINO_LED_IRRIGACAO_LINEAR GPIO_NUM_16
#define PINO_LED_IRRIGACAO_IA     GPIO_NUM_17

#define ENDERECO_LCD_PRINCIPAL 0x27
#define FREQUENCIA_I2C_HZ      100000
#define INTERVALO_LEITURA_MS   500
#define INTERVALO_AUTOTESTE_MS 300

static const char *TAG = "soil_ai";

static void imprimir_linha_console(const char *cor, char marcador)
{
    printf("%s", cor);
    for (int i = 0; i < 76; i++) {
        putchar(marcador);
    }
    printf("\033[0m\n");
}

static void imprimir_resumo_modelo_embarcado(bool ia_ok)
{
    const char *azul = "\033[1;36m";
    const char *verde = "\033[1;32m";
    const char *amarelo = "\033[1;33m";
    const char *magenta = "\033[1;35m";
    const char *reset = "\033[0m";

    imprimir_linha_console(magenta, '=');
    printf("%s  SISTEMA IA DE UMIDADE DO SOLO - MODELO EMBARCADO%s\n", magenta, reset);
    imprimir_linha_console(magenta, '=');
    printf("%s  Status do modelo :%s %s\n", verde, reset, ia_ok ? "carregado no ESP32-S3" : "indisponível");
    printf("%s  Modelo embarcado :%s %s\n", verde, reset, SOIL_MODEL_NAME);
    printf("%s  Amostras treino  :%s %d\n", verde, reset, SOIL_MODEL_SAMPLE_COUNT);
    printf("%s  Horizonte alvo   :%s %d minutos\n", verde, reset, SOIL_MODEL_HORIZON_MINUTES);
    printf("%s  Erro float MAE   :%s %.6f\n", verde, reset, (double)SOIL_MODEL_FLOAT_MAE);
    printf("%s  Erro int8 MAE    :%s %.6f\n", verde, reset, (double)SOIL_MODEL_INT8_MAE);

    imprimir_linha_console(azul, '-');
    printf("%s  Síntese do projeto%s\n", azul, reset);
    printf("  %-16s | %-52s\n", "Aspecto", "Configuração");
    imprimir_linha_console(azul, '-');
    printf("  %-16s | %-52s\n", "Entrada", "9 features normalizadas do cenário agrícola");
    printf("  %-16s | %-52s\n", "Saída", "nível previsto de umidade do solo 0..9");
    printf("  %-16s | %-52s\n", "Dados", "dataset Wazihub Soil Moisture Prediction");
    printf("  %-16s | %-52s\n", "Modelo", "rede neural compacta quantizada em int8");
    printf("  %-16s | %-52s\n", "Uso embarcado", "comparação entre irrigação por IA e linear");

    imprimir_linha_console(amarelo, '-');
    printf("%s  Features configuradas no modelo%s\n", amarelo, reset);
    printf("  %-2s %-20s %10s %10s %10s\n", "#", "Feature", "min", "default", "max");
    imprimir_linha_console(amarelo, '-');
    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        printf("  %2d %-20s %10.3f %10.3f %10.3f\n",
               i + 1,
               kSoilFeatureNames[i],
               (double)kSoilFeatureMin[i],
               (double)kSoilFeatureDefaults[i],
               (double)kSoilFeatureMax[i]);
    }
    imprimir_linha_console(magenta, '=');
}

static void imprimir_verificacao_hello_world_tflm(bool ia_ok)
{
    const char *verde = "\033[1;32m";
    const char *amarelo = "\033[1;33m";
    const char *cyan = "\033[1;36m";
    const char *reset = "\033[0m";

    imprimir_linha_console(cyan, '=');
    printf("%s  VERIFICAÇÃO 1 - PIPELINE TFLITE MICRO%s\n", cyan, reset);
    imprimir_linha_console(cyan, '=');
    printf("  %-4s %-34s %-32s\n", "Passo", "Etapa", "Resultado");
    imprimir_linha_console(cyan, '-');
    printf("  %-4d %-34s %-32s\n", 1, "modelo embarcado", ia_ok ? "carregado" : "indisponível");
    printf("  %-4d %-34s %-32s\n", 2, "entrada sintética", "umidade=45%, irrigação=N");
    printf("  %-4d %-34s %-32s\n", 3, "normalização", "min/max do treino Wazihub");

    if (!ia_ok) {
        printf("  %-4d %-34s %-32s\n", 4, "inferência", "não executada");
        imprimir_linha_console(amarelo, '=');
        return;
    }

    float features[SOIL_MODEL_INPUT_COUNT] = {0};
    float normalized[SOIL_MODEL_INPUT_COUNT] = {0};
    float prediction = 0.0f;
    soil_features_from_defaults(kSoilFeatureDefaults, 45.0f, false, features);
    soil_normalize_features(features, kSoilFeatureMin, kSoilFeatureMax, normalized);
    esp_err_t inferencia = soil_tflite_prever_nivel(normalized, &prediction);
    int nivel = soil_prediction_to_level(prediction);

    printf("  %-4d %-34s ", 4, "inferência");
    if (inferencia == ESP_OK) {
        printf("%snível=%d predição=%.3f%s\n", verde, nivel, (double)prediction, reset);
    } else {
        printf("%sfalhou: %s%s\n", amarelo, esp_err_to_name(inferencia), reset);
    }

    printf("\n  Checagem de sanidade do runtime TFLite Micro concluída.\n");
    imprimir_linha_console(cyan, '=');
}

static void imprimir_mapa_hardware(void)
{
    const char *magenta = "\033[1;35m";
    const char *reset = "\033[0m";

    imprimir_linha_console(magenta, '=');
    printf("%s  VERIFICAÇÃO 2 - LCDS, 7 SEGMENTOS E LEDS NO WOKWI%s\n", magenta, reset);
    imprimir_linha_console(magenta, '=');
    printf("  LCD I2C       : GPIO%d=SDA GPIO%d=SCL endereço=0x%02x -> medição por IA\n",
           PINO_SDA_LCD_PRINCIPAL, PINO_SCL_LCD_PRINCIPAL, ENDERECO_LCD_PRINCIPAL);
    printf("  LCD paralelo  : RS=%d E=%d D4=%d D5=%d D6=%d D7=%d -> medição linear\n",
           PINO_LCD_PAR_RS, PINO_LCD_PAR_E, PINO_LCD_PAR_D4, PINO_LCD_PAR_D5, PINO_LCD_PAR_D6, PINO_LCD_PAR_D7);
    printf("  7seg IA       : CD4511/BCD em GPIO%d,%d,%d,%d\n",
           PINO_BCD_A, PINO_BCD_B, PINO_BCD_C, PINO_BCD_D);
    printf("  7seg linear   : segmentos diretos em GPIO%d..GPIO%d e DP=GPIO%d\n",
           PINO_SEG_A, PINO_SEG_G, PINO_SEG_DP);
    printf("  LEDs          : azul linear=GPIO%d | dourado IA=GPIO%d\n",
           PINO_LED_IRRIGACAO_LINEAR, PINO_LED_IRRIGACAO_IA);
    imprimir_linha_console(magenta, '-');
}

static void executar_autoteste_visual(const bcd_display_t *display_ia,
                                      const seven_segment_display_t *display_ref,
                                      lcd1602_t *lcd_i2c,
                                      lcd1602_parallel_t *lcd_par)
{
    lcd1602_escrever_linha(lcd_i2c, 0, "TESTE LCD IA");
    lcd1602_escrever_linha(lcd_i2c, 1, "7SEG IA 0..9");
    lcd1602_parallel_escrever_linha(lcd_par, 0, "TESTE LCD LIN");
    lcd1602_parallel_escrever_linha(lcd_par, 1, "7SEG LIN 0..9");

    printf("  Autoteste visual: confira no Wokwi os LCDs e os dois displays.\n");
    printf("  %-8s | %-18s | %-18s | %-10s | %-12s\n",
           "Dígito", "LCD IA", "LCD LINEAR", "7seg IA", "7seg linear");
    imprimir_linha_console("\033[1;35m", '-');

    for (int digito = 0; digito <= 9; digito++) {
        char linha_ia[17] = {0};
        char linha_linear[17] = {0};
        snprintf(linha_ia, sizeof(linha_ia), "IA " SOIL_LCD_WORD_DIGITO " %d", digito);
        snprintf(linha_linear, sizeof(linha_linear), "LIN " SOIL_LCD_WORD_DIGITO " %d", digito);

        lcd1602_escrever_linha(lcd_i2c, 1, linha_ia);
        lcd1602_parallel_escrever_linha(lcd_par, 1, linha_linear);
        bcd_display_exibir(display_ia, digito);
        seven_segment_display_exibir(display_ref, digito);

        printf("  %-8d | IA dígito %-8d | LIN dígito %-7d | %-10d | %-12d\n",
               digito, digito, digito, digito, digito);
        vTaskDelay(pdMS_TO_TICKS(INTERVALO_AUTOTESTE_MS));
    }

    gpio_set_level(PINO_LED_IRRIGACAO_LINEAR, 1);
    gpio_set_level(PINO_LED_IRRIGACAO_IA, 1);
    printf("  LEDs: azul e dourado ligados por %d ms para conferência visual.\n", INTERVALO_AUTOTESTE_MS * 2);
    vTaskDelay(pdMS_TO_TICKS(INTERVALO_AUTOTESTE_MS * 2));
    gpio_set_level(PINO_LED_IRRIGACAO_LINEAR, 0);
    gpio_set_level(PINO_LED_IRRIGACAO_IA, 0);

    imprimir_linha_console("\033[1;35m", '=');
}

static void imprimir_cabecalho_monitoramento(void)
{
    const char *verde = "\033[1;32m";
    const char *reset = "\033[0m";

    imprimir_linha_console(verde, '=');
    printf("%s  VERIFICAÇÃO 3 - MONITORAMENTO EM TEMPO REAL%s\n", verde, reset);
    imprimir_linha_console(verde, '=');
    printf("  %-5s %-6s %-7s %-6s %-6s %-8s %-8s %-16s %-16s %-7s %-9s %-9s\n",
           "#", "ADC", "Umid%", "IA", "Lin", "IrrIA", "IrrLin", "LCD IA", "LCD linear", "7IA", "7Linear", "Final");
    imprimir_linha_console(verde, '-');
}

static void imprimir_linha_monitoramento(int amostra,
                                         int raw_adc,
                                         float percent,
                                         int nivel_ia,
                                         int nivel_linear,
                                         bool irrigar_ia,
                                         bool irrigar_linear,
                                         bool irrigar,
                                         const char *lcd_ia_l1,
                                         const char *lcd_linear_l1)
{
    printf("  %-5d %-6d %-7.0f %-6d %-6d %-8s %-8s %-16s %-16s %-7d %-9d %-9s\n",
           amostra,
           raw_adc,
           (double)percent,
           nivel_ia,
           nivel_linear,
           irrigar_ia ? "SIM" : "NÃO",
           irrigar_linear ? "SIM" : "NÃO",
           lcd_ia_l1,
           lcd_linear_l1,
           nivel_ia,
           nivel_linear,
           irrigar ? "SIM" : "NÃO");
}

extern "C" void app_main(void)
{
    lcd1602_t lcd_i2c = {};
    lcd1602_parallel_t lcd_par = {};
    soil_sensor_t sensor = {};

    bcd_display_t display_ia = {
        .pinos = {PINO_BCD_A, PINO_BCD_B, PINO_BCD_C, PINO_BCD_D},
    };
    seven_segment_display_t display_ref = {
        .pinos = {PINO_SEG_A, PINO_SEG_B, PINO_SEG_C, PINO_SEG_D, PINO_SEG_E, PINO_SEG_F, PINO_SEG_G},
        .ativo_alto = true,
    };

    lcd1602_config_t cfg_lcd_i2c = {
        .porta_i2c = PORTA_I2C_LCD_PRINCIPAL,
        .pino_sda = PINO_SDA_LCD_PRINCIPAL,
        .pino_scl = PINO_SCL_LCD_PRINCIPAL,
        .endereco = ENDERECO_LCD_PRINCIPAL,
        .frequencia_hz = FREQUENCIA_I2C_HZ,
    };
    lcd1602_parallel_config_t cfg_lcd_par = {
        .pino_rs = PINO_LCD_PAR_RS,
        .pino_e = PINO_LCD_PAR_E,
        .pinos_dados = {PINO_LCD_PAR_D4, PINO_LCD_PAR_D5, PINO_LCD_PAR_D6, PINO_LCD_PAR_D7},
    };

    ESP_LOGI(TAG, "Iniciando sistema...");

    ESP_ERROR_CHECK(lcd1602_iniciar(&lcd_i2c, &cfg_lcd_i2c));
    ESP_ERROR_CHECK(lcd1602_parallel_iniciar(&lcd_par, &cfg_lcd_par));
    ESP_ERROR_CHECK(soil_sensor_iniciar(&sensor, UNIDADE_SENSOR_UMIDADE, CANAL_SENSOR_UMIDADE));
    ESP_ERROR_CHECK(bcd_display_iniciar(&display_ia));
    ESP_ERROR_CHECK(seven_segment_display_iniciar(&display_ref));

    gpio_config_t config_indicadores = {
        .pin_bit_mask = (1ULL << PINO_SEG_DP) |
                        (1ULL << PINO_LED_IRRIGACAO_LINEAR) |
                        (1ULL << PINO_LED_IRRIGACAO_IA),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config_indicadores));
    ESP_ERROR_CHECK(gpio_set_level(PINO_SEG_DP, 0));
    ESP_ERROR_CHECK(gpio_set_level(PINO_LED_IRRIGACAO_LINEAR, 0));
    ESP_ERROR_CHECK(gpio_set_level(PINO_LED_IRRIGACAO_IA, 0));

    lcd1602_escrever_linha(&lcd_i2c, 0, SOIL_LCD_WORD_MEDICAO " IA");
    lcd1602_escrever_linha(&lcd_i2c, 1, "Carregando IA");
    lcd1602_parallel_escrever_linha(&lcd_par, 0, SOIL_LCD_WORD_MEDICAO " LINEAR");
    lcd1602_parallel_escrever_linha(&lcd_par, 1, "Carregando...");

    // Carregar modelo TFLite Micro
    esp_err_t tflite_status = soil_tflite_iniciar();
    bool ia_ok = (tflite_status == ESP_OK);

    if (!ia_ok) {
        ESP_LOGW(TAG, "Modelo indisponível: %s", esp_err_to_name(tflite_status));
        lcd1602_escrever_linha(&lcd_i2c, 1, "MODELO OFF");
        lcd1602_parallel_escrever_linha(&lcd_par, 1, "LINEAR OK");
    } else {
        ESP_LOGI(TAG, "Modelo pronto (%d features)", SOIL_MODEL_INPUT_COUNT);
        lcd1602_escrever_linha(&lcd_i2c, 1, "IA pronta!");
        lcd1602_parallel_escrever_linha(&lcd_par, 1, "LINEAR OK");
    }
    imprimir_resumo_modelo_embarcado(ia_ok);
    imprimir_verificacao_hello_world_tflm(ia_ok);
    imprimir_mapa_hardware();
    executar_autoteste_visual(&display_ia, &display_ref, &lcd_i2c, &lcd_par);
    imprimir_cabecalho_monitoramento();

    vTaskDelay(pdMS_TO_TICKS(1000));

    bool estado_pisca = false;
    int amostra = 0;

    // Loop principal
    while (true) {
        int raw_adc = 0;
        esp_err_t leitura = soil_sensor_ler_bruto(&sensor, &raw_adc);

        if (leitura != ESP_OK) {
            ESP_LOGW(TAG, "Sensor: %s", esp_err_to_name(leitura));
            lcd1602_escrever_linha(&lcd_i2c, 0, "ERRO SENSOR!");
            lcd1602_parallel_escrever_linha(&lcd_par, 0, "ERRO SENSOR!");
            gpio_set_level(PINO_SEG_DP, 0);
            gpio_set_level(PINO_LED_IRRIGACAO_LINEAR, 0);
            gpio_set_level(PINO_LED_IRRIGACAO_IA, 0);
            vTaskDelay(pdMS_TO_TICKS(INTERVALO_LEITURA_MS));
            continue;
        }

        float percent = soil_adc_to_percent(raw_adc, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
        int nivel_linear = soil_percent_to_level(percent);

        // Inferencia IA
        int nivel_ia = 0;
        bool usou_ia = false;

        if (ia_ok) {
            bool irrigando = nivel_linear <= 2;
            float features[SOIL_MODEL_INPUT_COUNT] = {0};
            float normalized[SOIL_MODEL_INPUT_COUNT] = {0};
            float prediction = 0.0f;

            soil_features_from_defaults(kSoilFeatureDefaults, percent, irrigando, features);
            soil_normalize_features(features, kSoilFeatureMin, kSoilFeatureMax, normalized);

            if (soil_tflite_prever_nivel(normalized, &prediction) == ESP_OK) {
                nivel_ia = soil_prediction_to_level(prediction);
                usou_ia = true;
            }
        }

        // Irrigação: cada medição tem seu indicador próprio.
        bool irrigar_linear = nivel_linear <= 2;
        bool irrigar_ia = usou_ia && nivel_ia <= 2;
        int nivel_final = usou_ia ? nivel_ia : nivel_linear;
        bool irrigar = nivel_final <= 2;
        estado_pisca = !estado_pisca;
        gpio_set_level(PINO_SEG_DP, irrigar ? 1 : 0);
        gpio_set_level(PINO_LED_IRRIGACAO_LINEAR, (irrigar_linear && estado_pisca) ? 1 : 0);
        gpio_set_level(PINO_LED_IRRIGACAO_IA, (irrigar_ia && estado_pisca) ? 1 : 0);

        // LCD I2C: medição baseada em IA
        char l0[17], l1[17];
        soil_format_lcd_lines(raw_adc, percent, nivel_ia, usou_ia, l0, l1);
        lcd1602_escrever_linha(&lcd_i2c, 0, l0);
        lcd1602_escrever_linha(&lcd_i2c, 1, l1);

        // LCD Paralelo: medição linear
        char p0[17], p1[17];
        soil_format_linear_lcd_lines(raw_adc, percent, nivel_linear, p0, p1);
        lcd1602_parallel_escrever_linha(&lcd_par, 0, p0);
        lcd1602_parallel_escrever_linha(&lcd_par, 1, p1);

        // digit1 = nível IA, digit2 = nível linear
        bcd_display_exibir(&display_ia, usou_ia ? nivel_ia : 0);
        seven_segment_display_exibir(&display_ref, nivel_linear);

        amostra++;
        if ((amostra % 20) == 1 && amostra > 1) {
            imprimir_cabecalho_monitoramento();
        }
        imprimir_linha_monitoramento(amostra,
                                     raw_adc,
                                     percent,
                                     nivel_ia,
                                     nivel_linear,
                                     irrigar_ia,
                                     irrigar_linear,
                                     irrigar,
                                     l1,
                                     p1);

        vTaskDelay(pdMS_TO_TICKS(INTERVALO_LEITURA_MS));
    }
}
