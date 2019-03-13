#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include <math.h>
#include <stdio.h>
#include "gtest/gtest.h"
#include "EbDefinitions.h"

// Macros
#ifndef PI
#define PI 3.141592653589793238462643383279502884
#endif

#ifndef TEST_GET_PARAM
#define TEST_GET_PARAM(k) std::tr1::get<k>(GetParam())
#endif

namespace svt_av1_test_tool {
INLINE int32_t round_shift(int64_t value, int32_t bit) {
    assert(bit >= 1);
    return (int32_t)((value + (1ll << (bit - 1))) >> bit);
}

INLINE int get_max_bit(int x) {
    int max_bit = -1;
    while (x) {
        x = x >> 1;
        max_bit++;
    }
    return max_bit;
}
}  // namespace svt_av1_test_tool

#endif  // _TEST_UTIL_H_
