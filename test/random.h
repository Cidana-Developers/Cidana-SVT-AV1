/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file random.h
 *
 * @brief Random generator for svt-av1 unit tests
 * - wrap C++11 random generator for different range.
 *
 * @author Cidana-Edmond, Cidana-Wenyao <wenyao.liu@cidana.com>
 *
 ******************************************************************************/

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

class SVTRandom {
  public:
    SVTRandom() : gen_(deterministic_seed_) {
    }

    explicit SVTRandom(uint32_t seed) : gen_(seed) {
    }

    void reset(uint32_t seed) {
        gen_.seed(seed);
    }

    uint32_t random_31(void) {
        return static_cast<uint32_t>(gen_uint_31_(gen_));
    }

    uint16_t random_16(void) {
        return static_cast<uint16_t>(gen_uint_16_(gen_));
    }

    uint16_t random_10(void) {
        return static_cast<uint16_t>(gen_uint_10_(gen_));
    }

    int16_t random_10s(void) {
        return static_cast<int16_t>(gen_int_10_(gen_));
    }

    int16_t random_9s(void) {
        return static_cast<int16_t>(gen_int_9_(gen_));
    }

    uint8_t random_8(void) {
        return static_cast<uint8_t>(gen_uint_8_(gen_));
    }

  private:
    const int deterministic_seed_{13596};
    std::mt19937 gen_;
    uniform_int_distribution<> gen_uint_8_{0, 0xFF};
    uniform_int_distribution<> gen_uint_10_{0, 0x3FF};
    uniform_int_distribution<> gen_uint_16_{0, 0xFFFF};
    uniform_int_distribution<> gen_uint_31_{0, 0x7FFFFFFF};
    uniform_int_distribution<> gen_int_9_{-256, 255};
    uniform_int_distribution<> gen_int_10_{-512, 511};
};

}  // namespace svt_av1_test_tool

#endif  // _TEST_RANDOM_H_
