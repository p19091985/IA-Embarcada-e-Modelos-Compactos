#include "soil_tflite_runner.h"

#include <algorithm>
#include <cstdint>

#include "esp_log.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "soil_model_data.h"

static const char *TAG_TFLITE = "soil_tflite";

namespace {

const tflite::Model *modelo = nullptr;
tflite::MicroInterpreter *interpretador = nullptr;
TfLiteTensor *entrada = nullptr;
TfLiteTensor *saida = nullptr;
bool pronto = false;

constexpr int kTensorArenaSize = 24 * 1024;
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

int8_t quantizar_int8(float valor, const TfLiteQuantizationParams &params)
{
    int quantizado = static_cast<int>(valor / params.scale + params.zero_point + (valor >= 0.0f ? 0.5f : -0.5f));
    quantizado = std::max(-128, std::min(127, quantizado));
    return static_cast<int8_t>(quantizado);
}

uint8_t quantizar_uint8(float valor, const TfLiteQuantizationParams &params)
{
    int quantizado = static_cast<int>(valor / params.scale + params.zero_point + (valor >= 0.0f ? 0.5f : -0.5f));
    quantizado = std::max(0, std::min(255, quantizado));
    return static_cast<uint8_t>(quantizado);
}

float ler_saida(const TfLiteTensor *tensor)
{
    if (tensor->type == kTfLiteFloat32) {
        return tensor->data.f[0];
    }
    if (tensor->type == kTfLiteInt8) {
        return (static_cast<int>(tensor->data.int8[0]) - tensor->params.zero_point) * tensor->params.scale;
    }
    if (tensor->type == kTfLiteUInt8) {
        return (static_cast<int>(tensor->data.uint8[0]) - tensor->params.zero_point) * tensor->params.scale;
    }

    ESP_LOGE(TAG_TFLITE, "tipo de saida nao suportado: %d", tensor->type);
    return 0.0f;
}

}  // namespace

esp_err_t soil_tflite_iniciar(void)
{
    if (pronto) {
        return ESP_OK;
    }

    modelo = tflite::GetModel(g_soil_moisture_model);
    if (modelo->version() != TFLITE_SCHEMA_VERSION) {
        ESP_LOGE(TAG_TFLITE, "schema do modelo %d difere do runtime %d", modelo->version(), TFLITE_SCHEMA_VERSION);
        return ESP_ERR_INVALID_VERSION;
    }

    static tflite::MicroMutableOpResolver<1> resolver;
    if (resolver.AddFullyConnected() != kTfLiteOk) {
        return ESP_FAIL;
    }

    static tflite::MicroInterpreter static_interpreter(modelo, resolver, tensor_arena, kTensorArenaSize);
    interpretador = &static_interpreter;

    if (interpretador->AllocateTensors() != kTfLiteOk) {
        ESP_LOGE(TAG_TFLITE, "AllocateTensors falhou");
        return ESP_ERR_NO_MEM;
    }

    entrada = interpretador->input(0);
    saida = interpretador->output(0);

    if (entrada->bytes < SOIL_MODEL_INPUT_COUNT) {
        ESP_LOGE(TAG_TFLITE, "entrada menor que o esperado");
        return ESP_ERR_INVALID_SIZE;
    }

    pronto = true;
    ESP_LOGI(TAG_TFLITE, "modelo TFLite Micro pronto: %d bytes", g_soil_moisture_model_len);
    return ESP_OK;
}

bool soil_tflite_pronto(void)
{
    return pronto;
}

esp_err_t soil_tflite_prever_nivel(const float normalized_features[SOIL_MODEL_INPUT_COUNT],
                                   float *predicted_level)
{
    if (!pronto || interpretador == nullptr || entrada == nullptr || saida == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }

    for (int i = 0; i < SOIL_MODEL_INPUT_COUNT; i++) {
        float valor = soil_clampf(normalized_features[i], 0.0f, 1.0f);

        if (entrada->type == kTfLiteFloat32) {
            entrada->data.f[i] = valor;
        } else if (entrada->type == kTfLiteInt8) {
            entrada->data.int8[i] = quantizar_int8(valor, entrada->params);
        } else if (entrada->type == kTfLiteUInt8) {
            entrada->data.uint8[i] = quantizar_uint8(valor, entrada->params);
        } else {
            ESP_LOGE(TAG_TFLITE, "tipo de entrada nao suportado: %d", entrada->type);
            return ESP_ERR_NOT_SUPPORTED;
        }
    }

    if (interpretador->Invoke() != kTfLiteOk) {
        ESP_LOGE(TAG_TFLITE, "Invoke falhou");
        return ESP_FAIL;
    }

    *predicted_level = ler_saida(saida);
    return ESP_OK;
}
