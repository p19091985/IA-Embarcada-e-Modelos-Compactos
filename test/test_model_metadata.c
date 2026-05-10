#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "soil_app_logic.h"
#include "soil_model_metadata.h"

/* Testes dos metadados do modelo TFLite.
   Garante que as constantes min/max/defaults estao consistentes. */

static void test_input_count_correto(void)
{
    /* O dataset Wazihub tem 9 features */
    assert(SOIL_MODEL_INPUT_COUNT == 9);
}

static void test_min_menor_que_max(void)
{
    /* Para cada feature, o valor minimo deve ser estritamente menor que o maximo.
       Isso garante que a normalizacao nao vai dividir por zero. */
    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        assert(kSoilFeatureMin[i] < kSoilFeatureMax[i]);
    }
}

static void test_defaults_dentro_do_range(void)
{
    /* Os valores default de cada feature devem estar dentro de [min, max].
       Se nao estiverem, a normalizacao vai produzir valores fora de [0, 1]. */
    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        assert(kSoilFeatureDefaults[i] >= kSoilFeatureMin[i]);
        assert(kSoilFeatureDefaults[i] <= kSoilFeatureMax[i]);
    }
}

static void test_feature_names_nao_nulos(void)
{
    /* Cada feature deve ter um nome nao-nulo */
    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        assert(kSoilFeatureNames[i] != NULL);
        assert(kSoilFeatureNames[i][0] != '\0');
    }
}

static void test_primeira_feature_e_umidade(void)
{
    /* A primeira feature deve ser sensor_humidity (compatibilidade com o legado) */
    assert(strcmp(kSoilFeatureNames[0], "sensor_humidity") == 0);
}

static void test_segunda_feature_e_irrigacao(void)
{
    /* A segunda feature deve ser irrigation (flag booleana 0/1) */
    assert(strcmp(kSoilFeatureNames[1], "irrigation") == 0);
    /* O range de irrigacao deve ser [0, 1] */
    assert(fabsf(kSoilFeatureMin[1] - 0.0f) < 0.001f);
    assert(fabsf(kSoilFeatureMax[1] - 1.0f) < 0.001f);
}

static void test_modelo_mae_positivo(void)
{
    /* O MAE deve ser positivo e razoavelmente pequeno */
    assert(SOIL_MODEL_FLOAT_MAE > 0.0f);
    assert(SOIL_MODEL_FLOAT_MAE < 1.0f);
    assert(SOIL_MODEL_INT8_MAE > 0.0f);
    assert(SOIL_MODEL_INT8_MAE < 1.0f);
}

static void test_normalizacao_com_metadados_reais(void)
{
    /* Usar os defaults do modelo para verificar que a normalizacao funciona */
    float features[SOIL_MODEL_INPUT_COUNT] = {0};
    float normalized[SOIL_MODEL_INPUT_COUNT] = {0};

    soil_features_from_defaults(kSoilFeatureDefaults, 50.0f, false, features);
    soil_normalize_features(features, kSoilFeatureMin, kSoilFeatureMax, normalized);

    /* Todos os valores normalizados devem estar entre 0 e 1 */
    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        assert(normalized[i] >= 0.0f);
        assert(normalized[i] <= 1.0f);
    }
}

int main(void)
{
    test_input_count_correto();
    test_min_menor_que_max();
    test_defaults_dentro_do_range();
    test_feature_names_nao_nulos();
    test_primeira_feature_e_umidade();
    test_segunda_feature_e_irrigacao();
    test_modelo_mae_positivo();
    test_normalizacao_com_metadados_reais();
    puts("test_model_metadata: ok");
    return 0;
}
