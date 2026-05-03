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
        code --reuse-window "$PASTA" "$PASTA/diagram.json" >/dev/null 2>&1 || true
    else
        printf "%b\n" "${YELLOW}Aviso:${RESET} Nao achei o comando 'code' para abrir o VS Code."
    fi
}

menu() {
    while true; do
        if [ -t 1 ]; then
            clear
        fi

        linha
        printf "%b\n" "${BOLD}${CYAN}IA Embarcada - Menu de Inicio${RESET}"
        linha
        printf "%b\n" "  ${GREEN}1${RESET}) Compilar rapido usando build incremental"
        printf "%b\n" "  ${YELLOW}2${RESET}) Limpar tudo e compilar do zero"
        printf "%b\n" "  ${MAGENTA}3${RESET}) Limpar, compilar do zero e abrir no VS Code"
        printf "%b\n" "  ${BLUE}4${RESET}) Abrir projeto e diagram.json no VS Code"
        printf "%b\n" "  ${RED}0${RESET}) Sair"
        linha
        printf "%b" "${BOLD}Escolha uma opcao:${RESET} "

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
    printf "\n%b\n" "Para rodar a simulacao, execute ${BOLD}Wokwi: Start Simulator${RESET} no VS Code."
    exit 0
fi

printf "%b\n" "${CYAN}==>${RESET} Carregando ESP-IDF de: $IDF_EXPORT"
if [ ! -f "$IDF_EXPORT" ]; then
    printf "%b\n" "${RED}Erro:${RESET} Nao achei o ESP-IDF em: $IDF_EXPORT"
    echo "Ajusta a variavel IDF_EXPORT se instalou em outro lugar."
    exit 1
fi

. "$IDF_EXPORT"

if [ "${CLEAN:-0}" = "1" ] || [ "$LIMPAR" = "1" ]; then
    if [ -d build ]; then
        printf "%b\n" "${CYAN}==>${RESET} Limpando build antigo..."
        idf.py fullclean
    else
        printf "%b\n" "${YELLOW}==>${RESET} Nada para limpar: build/ nao existe."
    fi
fi

printf "%b\n" "${CYAN}==>${RESET} Compilando..."
idf.py build

if [ "$ABRIR_VSCODE" = "1" ]; then
    abrir_vscode
fi

echo
printf "%b\n" "${GREEN}Build feito.${RESET} Agora abre o Wokwi Simulator no VS Code,"
printf "%b\n" "executa o comando '${BOLD}Wokwi: Start Simulator${RESET}'."
echo
echo "Arquivos gerados:"
echo "  - build/wokwi_dht_lcd.elf"
echo "  - build/flasher_args.json"
