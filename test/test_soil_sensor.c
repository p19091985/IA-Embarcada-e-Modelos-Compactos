#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "soil_app_logic.h"

/* Testa o range do ADC e conversao para percentual em condicoes extremas.
   O ESP32-S3 ADC retorna valores de 0 a 4095 (12 bits). */

static void test_sensor_adc_range_valido(void)
{
    /* valor zero deve dar 0% */
    assert(fabsf(soil_adc_to_percent(0, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX) - 0.0f) < 0.01f);

    /* valor maximo deve dar 100% */
    assert(fabsf(soil_adc_to_percent(SOIL_SENSOR_RAW_MAX, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX) - 100.0f) < 0.01f);

    /* metade do range deve dar ~50% */
    int meio = (SOIL_SENSOR_RAW_MAX - SOIL_SENSOR_RAW_MIN) / 2;
    float percent_meio = soil_adc_to_percent(meio, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
    assert(percent_meio > 40.0f && percent_meio < 60.0f);
}

static void test_sensor_adc_valor_negativo(void)
{
    /* leitura negativa (impossivel no hardware, mas devemos tratar) */
    float percent = soil_adc_to_percent(-100, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
    assert(fabsf(percent - 0.0f) < 0.01f);
}

static void test_sensor_adc_valor_acima_maximo(void)
{
    /* leitura acima do maximo deve ser clampada em 100% */
    float percent = soil_adc_to_percent(9999, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
    assert(fabsf(percent - 100.0f) < 0.01f);
}

static void test_sensor_range_invertido(void)
{
    /* se o range min >= max, deve retornar 0 por seguranca */
    assert(fabsf(soil_adc_to_percent(500, 1000, 1000) - 0.0f) < 0.01f);
    assert(fabsf(soil_adc_to_percent(500, 2000, 1000) - 0.0f) < 0.01f);
}

static void test_sensor_multiplos_pontos(void)
{
    /* verificar linearidade: 25%, 50%, 75% */
    int q1 = SOIL_SENSOR_RAW_MAX / 4;
    int q2 = SOIL_SENSOR_RAW_MAX / 2;
    int q3 = (SOIL_SENSOR_RAW_MAX * 3) / 4;

    float p1 = soil_adc_to_percent(q1, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
    float p2 = soil_adc_to_percent(q2, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);
    float p3 = soil_adc_to_percent(q3, SOIL_SENSOR_RAW_MIN, SOIL_SENSOR_RAW_MAX);

    assert(p1 > 20.0f && p1 < 30.0f);
    assert(p2 > 45.0f && p2 < 55.0f);
    assert(p3 > 70.0f && p3 < 80.0f);

    /* a ordem deve ser crescente */
    assert(p1 < p2);
    assert(p2 < p3);
}

int main(void)
{
    test_sensor_adc_range_valido();
    test_sensor_adc_valor_negativo();
    test_sensor_adc_valor_acima_maximo();
    test_sensor_range_invertido();
    test_sensor_multiplos_pontos();
    puts("test_soil_sensor: ok");
    return 0;
}
