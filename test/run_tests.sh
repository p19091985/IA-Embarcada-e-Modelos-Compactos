#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/ia_embarcada_tests"

mkdir -p "$BUILD_DIR"

echo "=== Compilando testes C ==="

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/soil_app_logic.c" \
    "$ROOT/test/test_soil_app_logic.c" \
    -lm \
    -o "$BUILD_DIR/test_soil_app_logic"

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/bcd_logic.c" \
    "$ROOT/test/test_bcd_logic.c" \
    -o "$BUILD_DIR/test_bcd_logic"

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/seven_segment_logic.c" \
    "$ROOT/test/test_seven_segment_logic.c" \
    -o "$BUILD_DIR/test_seven_segment_logic"

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/soil_app_logic.c" \
    "$ROOT/test/test_soil_sensor.c" \
    -lm \
    -o "$BUILD_DIR/test_soil_sensor"

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/soil_app_logic.c" \
    "$ROOT/test/test_lcd_format_edge_cases.c" \
    -lm \
    -o "$BUILD_DIR/test_lcd_format_edge_cases"

cc -std=c11 -Wall -Wextra -Werror -I"$ROOT/main" \
    "$ROOT/main/soil_app_logic.c" \
    "$ROOT/test/test_model_metadata.c" \
    -lm \
    -o "$BUILD_DIR/test_model_metadata"

echo "=== Executando testes C ==="

"$BUILD_DIR/test_soil_app_logic"
"$BUILD_DIR/test_bcd_logic"
"$BUILD_DIR/test_seven_segment_logic"
"$BUILD_DIR/test_soil_sensor"
"$BUILD_DIR/test_lcd_format_edge_cases"
"$BUILD_DIR/test_model_metadata"

echo "=== Executando testes Python ==="

python3 "$ROOT/test/test_training_dataset.py"
python3 "$ROOT/test/test_quantization_helpers.py"
python3 "$ROOT/test/test_model_inference.py"
python3 "$ROOT/test/test_diagram_wiring.py"

echo ""
echo "=== Todos os testes passaram! ==="
