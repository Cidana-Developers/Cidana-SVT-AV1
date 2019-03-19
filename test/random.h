
#ifndef _TEST_RANDOM_H_
#define _TEST_RANDOM_H_

#include <stdint.h>
#include "gtest/gtest.h"

namespace svt_av1_test_tool {

using testing::internal::Random;

class SVTRandom {
  public:
    SVTRandom() : _random(default_seed()) {
    }

    explicit SVTRandom(uint32_t seed) : _random(seed) {
    }

    void reset(uint32_t seed) {
        _random.Reseed(seed);
    }

    uint32_t random_31(void) {
        return _random.Generate(Random::kMaxRange);
    }

    uint16_t random_16(void) {
        const uint32_t value = _random.Generate(Random::kMaxRange);
        return (value >> 15) & 0xffff;
    }

    int16_t random_9s(void) {
        // use 1+8 bits: values between 255 (0x0FF) and -256 (0x100).
        const uint32_t value = _random.Generate(512);
        return 256 - static_cast<int16_t>(value);
    }

    uint8_t random_8(void) {
        const uint32_t value = _random.Generate(Random::kMaxRange);
        // there's a bit more entropy in the upper bits of this implementation.
        return (value >> 23) & 0xff;
    }

    uint8_t random_8_ex(void) {
        // returns a random value near 0 or near 255, to better exercise
        // saturation behavior.
        const uint8_t r = random_8();
        return r < 128 ? r << 4 : r >> 4;
    }

    int pseudo_uniform(int range) {
        return _random.Generate(range);
    }

    int operator()(int n) {
        return pseudo_uniform(n);
    }

  private:
    static int default_seed() {
        return 0xabcd;
    }

  private:
    Random _random;
};

}  // namespace svt_av1_test_tool

#endif  // _TEST_RANDOM_H_
