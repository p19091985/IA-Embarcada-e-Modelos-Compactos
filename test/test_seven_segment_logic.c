#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "seven_segment_logic.h"

static void assert_segments(int digit,
                            bool active_high,
                            uint8_t a,
                            uint8_t b,
                            uint8_t c,
                            uint8_t d,
                            uint8_t e,
                            uint8_t f,
                            uint8_t g)
{
    uint8_t segments[SEVEN_SEGMENT_COUNT] = {0};

    seven_segment_logic_encode_digit(digit, active_high, segments);

    assert(segments[0] == a);
    assert(segments[1] == b);
    assert(segments[2] == c);
    assert(segments[3] == d);
    assert(segments[4] == e);
    assert(segments[5] == f);
    assert(segments[6] == g);
}

static void test_clamp(void)
{
    assert(seven_segment_logic_clamp_digit(-1) == 0);
    assert(seven_segment_logic_clamp_digit(0) == 0);
    assert(seven_segment_logic_clamp_digit(8) == 8);
    assert(seven_segment_logic_clamp_digit(9) == 9);
    assert(seven_segment_logic_clamp_digit(10) == 9);
}

static void test_legacy_patterns_active_high(void)
{
    assert_segments(0, true, 1, 1, 1, 1, 1, 1, 0);
    assert_segments(1, true, 0, 1, 1, 0, 0, 0, 0);
    assert_segments(2, true, 1, 1, 0, 1, 1, 0, 1);
    assert_segments(3, true, 1, 1, 1, 1, 0, 0, 1);
    assert_segments(4, true, 0, 1, 1, 0, 0, 1, 1);
    assert_segments(5, true, 1, 0, 1, 1, 0, 1, 1);
    assert_segments(6, true, 1, 0, 1, 1, 1, 1, 1);
    assert_segments(7, true, 1, 1, 1, 0, 0, 0, 0);
    assert_segments(8, true, 1, 1, 1, 1, 1, 1, 1);
    assert_segments(9, true, 1, 1, 1, 1, 0, 1, 1);
}

static void test_active_low_inverts_legacy_pattern(void)
{
    assert_segments(2, false, 0, 0, 1, 0, 0, 1, 0);
    assert_segments(99, false, 0, 0, 0, 0, 1, 0, 0);
}

int main(void)
{
    test_clamp();
    test_legacy_patterns_active_high();
    test_active_low_inverts_legacy_pattern();
    puts("test_seven_segment_logic: ok");
    return 0;
}
