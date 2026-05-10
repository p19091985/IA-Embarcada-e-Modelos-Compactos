#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "bcd_logic.h"

static void assert_bits(int digit, uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    uint8_t bits[BCD_LOGIC_BIT_COUNT] = {0};

    bcd_logic_encode_digit(digit, bits);

    assert(bits[0] == a);
    assert(bits[1] == b);
    assert(bits[2] == c);
    assert(bits[3] == d);
}

static void test_clamp(void)
{
    assert(bcd_logic_clamp_digit(-1) == 0);
    assert(bcd_logic_clamp_digit(0) == 0);
    assert(bcd_logic_clamp_digit(5) == 5);
    assert(bcd_logic_clamp_digit(9) == 9);
    assert(bcd_logic_clamp_digit(10) == 9);
}

static void test_patterns(void)
{
    assert_bits(0, 0, 0, 0, 0);
    assert_bits(1, 1, 0, 0, 0);
    assert_bits(2, 0, 1, 0, 0);
    assert_bits(3, 1, 1, 0, 0);
    assert_bits(4, 0, 0, 1, 0);
    assert_bits(5, 1, 0, 1, 0);
    assert_bits(6, 0, 1, 1, 0);
    assert_bits(7, 1, 1, 1, 0);
    assert_bits(8, 0, 0, 0, 1);
    assert_bits(9, 1, 0, 0, 1);
    assert_bits(99, 1, 0, 0, 1);
}

int main(void)
{
    test_clamp();
    test_patterns();
    puts("test_bcd_logic: ok");
    return 0;
}

