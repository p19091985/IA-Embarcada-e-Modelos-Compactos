#include "bcd_logic.h"

int bcd_logic_clamp_digit(int digit)
{
    if (digit < 0) {
        return 0;
    }
    if (digit > 9) {
        return 9;
    }
    return digit;
}

void bcd_logic_encode_digit(int digit, uint8_t bits[BCD_LOGIC_BIT_COUNT])
{
    int safe_digit = bcd_logic_clamp_digit(digit);

    for (int bit = 0; bit < BCD_LOGIC_BIT_COUNT; bit++) {
        bits[bit] = (uint8_t)((safe_digit >> bit) & 0x01);
    }
}

