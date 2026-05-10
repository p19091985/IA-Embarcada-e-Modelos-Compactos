#include "seven_segment_logic.h"

static const uint8_t kLegacySevenSegmentPatterns[10][SEVEN_SEGMENT_COUNT] = {
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

int seven_segment_logic_clamp_digit(int digit)
{
    if (digit < 0) {
        return 0;
    }
    if (digit > 9) {
        return 9;
    }
    return digit;
}

void seven_segment_logic_encode_digit(int digit,
                                      bool active_high,
                                      uint8_t segments[SEVEN_SEGMENT_COUNT])
{
    int safe_digit = seven_segment_logic_clamp_digit(digit);

    for (int segment = 0; segment < SEVEN_SEGMENT_COUNT; segment++) {
        uint8_t on = kLegacySevenSegmentPatterns[safe_digit][segment];
        segments[segment] = active_high ? on : (uint8_t)!on;
    }
}
