# IA Embarcada e Modelos Compactos

Repositório de estudos sobre IA embarcada, modelos compactos e aplicações em microcontroladores. O `main` funciona como índice geral; cada atividade ou experimento deve ficar organizado em um branch próprio.

## Índice

- [Branches do repositório](#branches-do-repositório)
- [Estado da arte](#estado-da-arte)
- [Como este repositório se encaixa](#como-este-repositório-se-encaixa)
- [Roteiro sugerido](#roteiro-sugerido)
- [Fontes de referência](#fontes-de-referência)
- [Licença](#licença)

## Branches do repositório

| Branch | Papel no repositório | Conteúdo principal | Acesso |
|:---|:---|:---|:---|
| `main` | Índice e documentação central | README, licença e visão geral do projeto | [`main`](https://github.com/p19091985/IA-Embarcada-e-Modelos-Compactos/tree/main) |
| `Atividade-Avaliativa-Prática---Leitura-de-sensor` | Atividade prática com ESP32-S3 | Leitura de DHT22, exibição em LCD1602 I2C, simulação Wokwi e ESP-IDF | [`Atividade-Avaliativa-Prática---Leitura-de-sensor`](https://github.com/p19091985/IA-Embarcada-e-Modelos-Compactos/tree/Atividade-Avaliativa-Pr%C3%A1tica---Leitura-de-sensor) |

Para abrir o branch da atividade localmente:

```bash
git switch Atividade-Avaliativa-Prática---Leitura-de-sensor
```

## Estado da arte

IA embarcada e TinyML buscam executar inferência perto do sensor, com baixa latência, menor dependência de rede e melhor privacidade. Em microcontroladores, o desafio central é equilibrar acurácia, consumo de energia, memória RAM/flash e tempo de inferência.

As práticas mais usadas atualmente são:

- **Quantização**: conversão de pesos e ativações para formatos menores, como INT8, para reduzir memória e acelerar a inferência.
- **Modelos compactos**: arquiteturas pequenas, redes convolucionais leves e modelos treinados para tarefas específicas do dispositivo.
- **Pruning e distilação**: remoção de parâmetros pouco relevantes e transferência de conhecimento de modelos maiores para menores.
- **Kernels otimizados por hardware**: uso de bibliotecas como ESP-NN, ESP-DL e CMSIS-NN para acelerar operações de redes neurais em chips específicos.
- **Compilação para o alvo**: geração de código C/C++ ou formatos próprios do dispositivo para reduzir overhead de runtime.
- **Medição contínua**: validação de latência, uso de RAM, tamanho do firmware, consumo energético e acurácia antes de considerar um modelo pronto.

No ecossistema ESP32, o ESP32-S3 é um alvo interessante para estudos porque combina Wi-Fi/Bluetooth, suporte ao ESP-IDF, instruções úteis para DSP/IA e integração com bibliotecas da Espressif. Para workloads mais exigentes, o estado da arte também inclui NPUs e extensões vetoriais em famílias como Arm Cortex-M com Ethos-U.

## Como este repositório se encaixa

O branch `Atividade-Avaliativa-Prática---Leitura-de-sensor` cobre a base necessária para IA embarcada: aquisição de dados confiável em um microcontrolador. A atividade lê temperatura e umidade com um DHT22, mostra os dados em um LCD1602 via I2C e registra as leituras no serial usando ESP-IDF e Wokwi.

Essa etapa ainda não executa um modelo de IA, mas prepara o caminho para isso. A evolução natural é transformar as leituras do sensor em dataset, treinar um modelo compacto fora do dispositivo e depois embarcar a inferência no ESP32-S3.

## Roteiro sugerido

1. Capturar leituras reais ou simuladas do sensor e salvar um dataset com rótulos.
2. Definir uma tarefa pequena, como classificação de conforto térmico, detecção de anomalia ou previsão simples.
3. Treinar um modelo compacto e comparar com uma regra manual de referência.
4. Quantizar o modelo e medir perda de acurácia.
5. Embarcar a inferência usando TensorFlow Lite for Microcontrollers, ESP-DL ou uma implementação C/C++ dedicada.
6. Registrar métricas no README do branch: tamanho do firmware, RAM usada, latência média, acurácia e limitações.

## Fontes de referência

- [TensorFlow Lite for Microcontrollers](https://www.tensorflow.org/lite/microcontrollers/overview)
- [ESP-DL, biblioteca de deep learning da Espressif](https://docs.espressif.com/projects/esp-dl/en/release-v1.1/esp32s3/index.html)
- [ESP-NN, kernels otimizados para chips Espressif](https://github.com/espressif/esp-nn)
- [Arm Edge AI em Cortex-M e Ethos-U](https://developer.arm.com/edge-ai/arm-cortex-m-and-ethos-u)
- [Edge Impulse EON Compiler](https://docs.edgeimpulse.com/studio/projects/deployment/eon-compiler)

## Licença

MIT - consulte o arquivo [`LICENSE`](LICENSE).
