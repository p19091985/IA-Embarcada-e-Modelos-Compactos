#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BCD_LOGIC_BIT_COUNT 4

int bcd_logic_clamp_digit(int digit);
void bcd_logic_encode_digit(int digit, uint8_t bits[BCD_LOGIC_BIT_COUNT]);

#ifdef __cplusplus
}
#endif

