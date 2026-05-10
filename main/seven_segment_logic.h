#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEVEN_SEGMENT_COUNT 7

int seven_segment_logic_clamp_digit(int digit);
void seven_segment_logic_encode_digit(int digit,
                                      bool active_high,
                                      uint8_t segments[SEVEN_SEGMENT_COUNT]);

#ifdef __cplusplus
}
#endif
