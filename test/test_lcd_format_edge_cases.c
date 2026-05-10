#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "soil_app_logic.h"

/* Testes de formatacao LCD com valores extremos e limites.
   O LCD tem 16 colunas por linha, entao cada string deve ter <= 16 chars. */

static void test_formato_nivel_zero(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_lcd_lines(0, 0.0f, 0, true, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strcmp(linha0, SOIL_LCD_WORD_MEDICAO " IA N:0") == 0);
    assert(strstr(linha1, "0%") != NULL);
}

static void test_formato_nivel_maximo(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_lcd_lines(3504, 100.0f, 9, true, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strcmp(linha0, SOIL_LCD_WORD_MEDICAO " IA N:9") == 0);
}

static void test_formato_sem_ia(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_lcd_lines(1000, 28.5f, 3, false, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strcmp(linha0, SOIL_LCD_WORD_MEDICAO " IA") == 0);
    assert(strstr(linha1, "MODELO OFF") != NULL);
}

static void test_formato_adc_maximo_4095(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    /* ADC de 4 digitos — nao pode estourar o buffer de 16 chars */
    soil_format_lcd_lines(4095, 100.0f, 9, true, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
}

static void test_formato_adc_zero(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_lcd_lines(0, 0.0f, 0, false, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strstr(linha1, "0") != NULL);
}

static void test_formato_nivel_intermediario(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_lcd_lines(1752, 50.0f, 5, true, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strcmp(linha0, SOIL_LCD_WORD_MEDICAO " IA N:5") == 0);
    assert(strstr(linha1, "1752") != NULL);
}

static void test_formato_linear(void)
{
    char linha0[17] = {0};
    char linha1[17] = {0};

    soil_format_linear_lcd_lines(3504, 100.0f, 9, linha0, linha1);

    assert(strlen(linha0) <= 16);
    assert(strlen(linha1) <= 16);
    assert(strcmp(linha0, SOIL_LCD_WORD_MEDICAO " LINEAR") == 0);
    assert(strcmp(linha1, "N:9 100% ADC3504") == 0);
}

int main(void)
{
    test_formato_nivel_zero();
    test_formato_nivel_maximo();
    test_formato_sem_ia();
    test_formato_adc_maximo_4095();
    test_formato_adc_zero();
    test_formato_nivel_intermediario();
    test_formato_linear();
    puts("test_lcd_format_edge_cases: ok");
    return 0;
}
