# Testes

Os testes daqui cobrem a parte que pode ser validada sem placa:

- conversao ADC -> porcentagem de umidade
- conversao porcentagem/predicao -> nivel `0..9`
- montagem e normalizacao das entradas do modelo
- formatacao das duas linhas do LCD
- codificacao BCD do display legado
- matriz direta do segundo display de 7 segmentos legado
- presenca do dataset e dos arquivos de modelo gerados

Para executar:

```bash
./test/run_tests.sh
```

O firmware completo ainda precisa ser validado com ESP-IDF/Wokwi porque os drivers
de ADC, GPIO, LCD e TFLite Micro dependem do alvo ESP32-S3.
