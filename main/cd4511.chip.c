#include "wokwi-api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    pin_t input_pins[4];
    pin_t control_lt;
    pin_t control_bl;
    pin_t control_le;
    pin_t segment_pins[7];
    uint8_t latched_digit;
} chip_state_t;

static const uint8_t kSegments[10][7] = {
    {1, 1, 1, 1, 1, 1, 0},
    {0, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 1, 1, 0, 1},
    {1, 1, 1, 1, 0, 0, 1},
    {0, 1, 1, 0, 0, 1, 1},
    {1, 0, 1, 1, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 1, 1},
};

static uint8_t read_digit(chip_state_t *chip)
{
    uint8_t digit = 0;

    for (int bit = 0; bit < 4; bit++) {
        if (pin_read(chip->input_pins[bit]) == HIGH) {
            digit |= (uint8_t)(1 << bit);
        }
    }

    return digit;
}

static void write_segments(chip_state_t *chip, uint8_t digit)
{
    bool lamp_test = pin_read(chip->control_lt) == LOW;
    bool blank = pin_read(chip->control_bl) == LOW;

    for (int segment = 0; segment < 7; segment++) {
        uint8_t on = 0;

        if (lamp_test) {
            on = 1;
        } else if (!blank && digit < 10) {
            on = kSegments[digit][segment];
        }

        pin_write(chip->segment_pins[segment], on ? HIGH : LOW);
    }
}

static void update_display(chip_state_t *chip)
{
    if (pin_read(chip->control_le) == LOW) {
        chip->latched_digit = read_digit(chip);
    }

    write_segments(chip, chip->latched_digit);
}

static void on_pin_change(void *user_data, pin_t pin, uint32_t value)
{
    (void)pin;
    (void)value;

    chip_state_t *chip = (chip_state_t *)user_data;
    update_display(chip);
}

void chip_init(void)
{
    chip_state_t *chip = malloc(sizeof(chip_state_t));
    if (chip == NULL) {
        return;
    }
    memset(chip, 0, sizeof(chip_state_t));

    chip->input_pins[0] = pin_init("A", INPUT_PULLDOWN);
    chip->input_pins[1] = pin_init("B", INPUT_PULLDOWN);
    chip->input_pins[2] = pin_init("C", INPUT_PULLDOWN);
    chip->input_pins[3] = pin_init("D", INPUT_PULLDOWN);
    chip->control_lt = pin_init("LT", INPUT_PULLUP);
    chip->control_bl = pin_init("BL", INPUT_PULLUP);
    chip->control_le = pin_init("LE", INPUT_PULLDOWN);
    chip->segment_pins[0] = pin_init("SEG_A", OUTPUT_LOW);
    chip->segment_pins[1] = pin_init("SEG_B", OUTPUT_LOW);
    chip->segment_pins[2] = pin_init("SEG_C", OUTPUT_LOW);
    chip->segment_pins[3] = pin_init("SEG_D", OUTPUT_LOW);
    chip->segment_pins[4] = pin_init("SEG_E", OUTPUT_LOW);
    chip->segment_pins[5] = pin_init("SEG_F", OUTPUT_LOW);
    chip->segment_pins[6] = pin_init("SEG_G", OUTPUT_LOW);

    pin_watch_config_t watch_config = {
        .edge = BOTH,
        .pin_change = on_pin_change,
        .user_data = chip,
    };

    for (int bit = 0; bit < 4; bit++) {
        pin_watch(chip->input_pins[bit], &watch_config);
    }
    pin_watch(chip->control_lt, &watch_config);
    pin_watch(chip->control_bl, &watch_config);
    pin_watch(chip->control_le, &watch_config);

    update_display(chip);
}
