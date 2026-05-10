#!/usr/bin/env bash

if [ -z "${BASH_VERSION:-}" ]; then
    exec bash "$0" "$@"
fi

set -e

PASTA="$(cd "$(dirname "$0")" && pwd)"
LIMPAR=0
ABRIR_VSCODE=0
SO_ABRIR_VSCODE=0
USAR_MENU=0

if [ -t 1 ] && command -v tput >/dev/null 2>&1; then
    CORES="$(tput colors 2>/dev/null || echo 0)"
else
    CORES=0
fi

if [ "$CORES" -ge 8 ] 2>/dev/null; then
    RESET="$(tput sgr0)"
    BOLD="$(tput bold)"
    DIM="$(tput dim)"
    RED="$(tput setaf 1)"
    GREEN="$(tput setaf 2)"
    YELLOW="$(tput setaf 3)"
    BLUE="$(tput setaf 4)"
    MAGENTA="$(tput setaf 5)"
    CYAN="$(tput setaf 6)"
else
    RESET=""
    BOLD=""
    DIM=""
    RED=""
    GREEN=""
    YELLOW=""
    BLUE=""
    MAGENTA=""
    CYAN=""
fi

linha() {
    printf "%b\n" "${DIM}------------------------------------------------------------${RESET}"
}

uso() {
    printf "%b\n" "${BOLD}Uso:${RESET}"
    echo "  ./iniciar.sh                 abre o menu interativo"
    echo "  ./iniciar.sh menu            abre o menu interativo"
    echo "  ./iniciar.sh limpar          limpa com idf.py fullclean e compila"
    echo "  ./iniciar.sh vscode          limpa, compila do zero e abre o diagrama no VS Code"
    echo "  ./iniciar.sh limpar vscode   limpa, compila e abre o diagrama"
    echo "  CLEAN=1 ./iniciar.sh         limpa com idf.py fullclean e compila"
}

abrir_vscode() {
    if command -v code >/dev/null 2>&1; then
        printf "%b\n" "${CYAN}==>${RESET} Abrindo projeto e diagram.json no VS Code..."
        if ! code --new-window "$PASTA" --goto "$PASTA/diagram.json:1"; then
            printf "%b\n" "${RED}Erro:${RESET} O comando 'code' falhou ao abrir o VS Code."
            printf "%b\n" "Tente manualmente: ${BOLD}code --new-window \"$PASTA\" --goto \"$PASTA/diagram.json:1\"${RESET}"
            return 1
        fi
    else
        printf "%b\n" "${YELLOW}Aviso:${RESET} Não achei o comando 'code' para abrir o VS Code."
        printf "%b\n" "Tente abrir manualmente pelo aplicativo ou instale o comando Shell Command: Install 'code' command in PATH."
        return 1
    fi
}

preparar_python_esp_idf() {
    local python_preferido=""
    local versao_python=""
    local env_python=""
    local pasta_shim="$HOME/.espressif/ia_embarcada_idf_python"

    if [ -x "$HOME/.local/bin/python3.11" ]; then
        python_preferido="$HOME/.local/bin/python3.11"
    elif command -v python3.11 >/dev/null 2>&1; then
        python_preferido="$(command -v python3.11)"
    fi

    if [ -z "$python_preferido" ]; then
        return
    fi

    versao_python="$("$python_preferido" -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')"
    env_python="$HOME/.espressif/python_env/idf6.0_py${versao_python}_env/bin"

    mkdir -p "$pasta_shim"
    ln -sf "$python_preferido" "$pasta_shim/python3"

    export PATH="$pasta_shim:$PATH"
    if [ -d "$env_python" ]; then
        if [ ! -x "$env_python/python" ] || [ ! -x "$env_python/python3" ]; then
            printf "%b\n" "${YELLOW}==>${RESET} Reparando virtualenv Python do ESP-IDF..."
            ln -sf "$python_preferido" "$env_python/python3"
            ln -sf python3 "$env_python/python"
            ln -sf python3 "$env_python/python${versao_python}"
        fi
        export PATH="$env_python:$PATH"
    fi

    printf "%b\n" "${CYAN}==>${RESET} Usando Python $versao_python para o ESP-IDF: $python_preferido"
}

preparar_modelo_e_dataset() {
    local dataset_csv="$PASTA/datasets/Wazihub_Soil_Moisture_Prediction/Dataset/Train.csv"
    local modelo_int8="$PASTA/main/models/soil_moisture_level_int8.tflite"
    local modelo_float="$PASTA/main/models/soil_moisture_level_float.tflite"
    local modelo_cc="$PASTA/main/soil_model_data.cc"
    local modelo_meta="$PASTA/main/soil_model_metadata.h"
    local python_treino="$PASTA/.venv-training/bin/python"
    local python_base=""
    local precisa_treinar=0

    if [ ! -f "$dataset_csv" ]; then
        printf "%b\n" "${YELLOW}==>${RESET} Dataset local não encontrado; ele será baixado antes do treino."
        precisa_treinar=1
    fi

    for artefato in "$modelo_int8" "$modelo_float" "$modelo_cc" "$modelo_meta"; do
        if [ ! -f "$artefato" ]; then
            printf "%b\n" "${YELLOW}==>${RESET} Artefato ausente: ${artefato#$PASTA/}"
            precisa_treinar=1
        fi
    done

    if [ "$precisa_treinar" != "1" ]; then
        return
    fi

    if [ ! -x "$python_treino" ]; then
        if [ -x "$HOME/.local/bin/python3.11" ]; then
            python_base="$HOME/.local/bin/python3.11"
        elif command -v python3.11 >/dev/null 2>&1; then
            python_base="$(command -v python3.11)"
        elif command -v python3 >/dev/null 2>&1; then
            python_base="$(command -v python3)"
        else
            printf "%b\n" "${RED}Erro:${RESET} Não encontrei Python para criar .venv-training."
            exit 1
        fi

        printf "%b\n" "${CYAN}==>${RESET} Criando ambiente de treino em .venv-training..."
        "$python_base" -m venv "$PASTA/.venv-training"
    fi

    if ! "$python_treino" -c 'import tensorflow, pandas, sklearn, matplotlib, absl' >/dev/null 2>&1; then
        printf "%b\n" "${CYAN}==>${RESET} Instalando dependências de treino..."
        "$python_treino" -m pip install -r "$PASTA/tools/requirements-training.txt"
    fi

    printf "%b\n" "${CYAN}==>${RESET} Baixando dataset, treinando e gerando modelo embarcado..."
    "$python_treino" "$PASTA/tools/train_soil_moisture_tflite.py" --epochs "${TRAIN_EPOCHS:-60}"
}

menu() {
    while true; do
        if [ -t 1 ]; then
            clear
        fi

        linha
        printf "%b\n" "${BOLD}${CYAN}IA Embarcada - Menu de Início${RESET}"
        linha
        printf "%b\n" "  ${GREEN}1${RESET}) Compilar rápido usando build incremental"
        printf "%b\n" "  ${YELLOW}2${RESET}) Limpar tudo e compilar do zero"
        printf "%b\n" "  ${MAGENTA}3${RESET}) Limpar, compilar do zero e abrir no VS Code"
        printf "%b\n" "  ${BLUE}4${RESET}) Abrir projeto e diagram.json no VS Code"
        printf "%b\n" "  ${RED}0${RESET}) Sair"
        linha
        printf "%b" "${BOLD}Escolha uma opção:${RESET} "

        IFS= read -r opcao || exit 0

        case "$opcao" in
            1)
                LIMPAR=0
                ABRIR_VSCODE=0
                SO_ABRIR_VSCODE=0
                return
                ;;
            2)
                LIMPAR=1
                ABRIR_VSCODE=0
                SO_ABRIR_VSCODE=0
                return
                ;;
            3)
                LIMPAR=1
                ABRIR_VSCODE=1
                SO_ABRIR_VSCODE=0
                return
                ;;
            4)
                LIMPAR=0
                ABRIR_VSCODE=0
                SO_ABRIR_VSCODE=1
                return
                ;;
            0|s|S|sair|Sair|SAIR|q|Q)
                printf "%b\n" "${GREEN}Saindo. Nada foi alterado.${RESET}"
                exit 0
                ;;
            *)
                printf "%b\n" "${RED}Opcao invalida.${RESET}"
                sleep 1
                ;;
        esac
    done
}

for arg in "$@"; do
    case "$arg" in
        limpar|/limpar|clean|/clean|--clean|-c)
            LIMPAR=1
            ;;
        vscode|abrir|open|--vscode|--open)
            ABRIR_VSCODE=1
            LIMPAR=1
            ;;
        menu|--menu|-m)
            USAR_MENU=1
            ;;
        ajuda|help|--help|-h)
            uso
            exit 0
            ;;
        *)
            echo "Opcao desconhecida: $arg"
            uso
            exit 1
            ;;
    esac
done

if { [ "$#" -eq 0 ] && [ "${CLEAN:-0}" != "1" ]; } || [ "$USAR_MENU" = "1" ]; then
    menu
fi

if [ -z "${IDF_EXPORT:-}" ]; then
    if [ -n "${IDF_PATH:-}" ]; then
        IDF_EXPORT="$IDF_PATH/export.sh"
    else
        IDF_EXPORT="$HOME/.espressif/v6.0.1/esp-idf/export.sh"
    fi
fi

cd "$PASTA"

if [ "$SO_ABRIR_VSCODE" = "1" ]; then
    abrir_vscode
    printf "\n%b\n" "Para rodar a simulação, execute ${BOLD}Wokwi: Start Simulator${RESET} no VS Code."
    exit 0
fi

printf "%b\n" "${CYAN}==>${RESET} Carregando ESP-IDF de: $IDF_EXPORT"
if [ ! -f "$IDF_EXPORT" ]; then
    printf "%b\n" "${RED}Erro:${RESET} Não achei o ESP-IDF em: $IDF_EXPORT"
    echo "Ajuste a variável IDF_EXPORT se instalou em outro lugar."
    exit 1
fi

preparar_python_esp_idf
. "$IDF_EXPORT"
preparar_modelo_e_dataset

if [ "${CLEAN:-0}" = "1" ] || [ "$LIMPAR" = "1" ]; then
    if [ -d build ]; then
        printf "%b\n" "${CYAN}==>${RESET} Limpando build antigo..."
        rm -rf build
    fi
    if [ -d managed_components ]; then
        rm -rf managed_components
    fi
fi

printf "%b\n" "${CYAN}==>${RESET} Compilando..."

# Primeiro: reconfigure para baixar managed_components (se necessário)
idf.py reconfigure 2>/dev/null || true

# Patch: desabilitar ESP_NN (assembly SIMD que Wokwi nao emula)
TFLITE_CMAKE="$PASTA/managed_components/espressif__esp-tflite-micro/CMakeLists.txt"
if [ -f "$TFLITE_CMAKE" ] && grep -q 'PRIVATE -DESP_NN)' "$TFLITE_CMAKE" && ! grep -q '# DESABILITADO' "$TFLITE_CMAKE"; then
    printf "%b\n" "${YELLOW}==>${RESET} Desabilitando ESP_NN (incompatível com Wokwi)..."
    sed -i 's|target_compile_options(${COMPONENT_LIB} PRIVATE -DESP_NN)|# DESABILITADO para Wokwi\n# target_compile_options(${COMPONENT_LIB} PRIVATE -DESP_NN)|' "$TFLITE_CMAKE"
fi

idf.py build

if [ "$ABRIR_VSCODE" = "1" ]; then
    abrir_vscode
fi

echo
printf "%b\n" "${GREEN}Build feito.${RESET} Agora abre o Wokwi Simulator no VS Code,"
printf "%b\n" "executa o comando '${BOLD}Wokwi: Start Simulator${RESET}'."
echo
echo "Arquivos gerados:"
echo "  - build/wokwi_soil_ai.elf"
echo "  - build/flasher_args.json"
