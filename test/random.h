
#ifndef _TEST_RANDOM_H_
#define _TEST_RANDOM_H_

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <random>
#include "gtest/gtest.h"

namespace svt_av1_test_tool {

using std::mt19937;
using std::uniform_int_distribution;

static uniform_int_distribution<> gen_uint_8(0, 0xFF);
static uniform_int_distribution<> gen_uint_10(0, 0x3FF);
static uniform_int_distribution<> gen_uint_16(0, 0xFFFF);
static uniform_int_distribution<> gen_uint_31(0, 0x7FFFFFFF);
static uniform_int_distribution<> gen_int_9(-256, 255);
static uniform_int_distribution<> gen_int_10(-512, 511);

class SVTRandom {
  public:
    SVTRandom() : _random(default_seed()) {
    }

    explicit SVTRandom(uint32_t seed) : _random(seed) {
    }

    void reset(uint32_t seed) {
        _random.seed(seed);
    }

    uint32_t random_31(void) {
        return static_cast<uint32_t>(gen_uint_31(_random));
    }

    uint16_t random_16(void) {
        return static_cast<int16_t>(gen_uint_16(_random));
    }

    uint16_t random_10(void) {
        return static_cast<uint16_t>(gen_uint_10(_random));
    }

    int16_t random_10s(void) {
        return static_cast<int16_t>(gen_int_10(_random));
    }

    int16_t random_9s(void) {
        // use 1+8 bits: values between 255 (0x0FF) and -256 (0x100).
        return static_cast<int16_t>(gen_int_9(_random));
    }

    uint8_t random_8(void) {
        return static_cast<uint8_t>(gen_uint_8(_random));
    }

    uint8_t random_8_ex(void) {
        // returns a random value near 0 or near 255, to better exercise
        // saturation behavior.
        const uint8_t r = random_8();
        return r < 128 ? r << 4 : r >> 4;
    }

    int pseudo_uniform(int range) {
        uniform_int_distribution<> generator(0, range);
        return generator(_random);
    }

    int operator()(int n) {
        return pseudo_uniform(n);
    }

  private:
    static unsigned int default_seed() {
        return mt19937::default_seed;
    }

  private:
    mt19937 _random;
};

}  // namespace svt_av1_test_tool

#endif  // _TEST_RANDOM_H_
