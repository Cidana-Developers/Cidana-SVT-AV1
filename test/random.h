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
#include <assert.h>
#include <math.h>
#include <random>

/** @defgroup svt_av1_test_tool Tool set of test
 *  Defines the tool set of unit test such as random generator, bits shifting
 * and etc...
 *  @{
 */

namespace svt_av1_test_tool {

using std::mt19937;
using std::uniform_int_distribution;

/** SVTRandom defines a tool class for generating random integer as uint test
 * samples*/
class SVTRandom {
  public:
    /** contructor with given minimum and maximum bound of random integer*/
    SVTRandom(const int min_bound, const int max_bound)
        : gen_(deterministic_seed_) {
        setup(min_bound, max_bound);
    }

    /** contructor with given limit bits and signed symbol*/
    SVTRandom(const int nbits, const bool is_signed)
        : gen_(deterministic_seed_) {
        int set_bits = is_signed ? nbits - 1 : nbits;
        assert(set_bits < 32);
        int min_bound = 0, max_bound = 0;
        for (int i = 0; i < set_bits; i++)
            max_bound |= (1 << i);
        if (is_signed)
            min_bound = 0 - (1 << set_bits);
        setup(min_bound, max_bound);
        return;
    }

    /** contructor with given minimum, maximum bound of random integer and seed
     */
    explicit SVTRandom(const int min_bound, const int max_bound,
                       const uint32_t seed)
        : gen_(seed) {
        setup(min_bound, max_bound);
    }

    /** contructor with given limit bits, signed symbol and seed */
    explicit SVTRandom(const int nbits, const bool is_signed,
                       const uint32_t seed)
        : gen_(seed) {
        assert(is_signed ? nbits < 31 : nbits <= 31);
        int set_bits = is_signed ? nbits - 1 : nbits;
        int min_bound = 0, max_bound = 0;
        for (int i = 0; i < set_bits; i++)
            max_bound |= (1 << i);
        if (is_signed)
            min_bound = 0 - max_bound;
        setup(min_bound, max_bound);
    }

    /** reset generator with new seed
     * @param seed new seed for generator reset
     */
    void reset(uint32_t seed) {
        gen_.seed(seed);
    }

    /** generate a new random integer with minimum and maximum bounds
     * @return:
     * value of random integer
     */
    int random() {
        return dist_nbit_(gen_);
    }

  private:
    void setup(const int min_bound, const int max_bound) {
        assert(min_bound <= max_bound);
        decltype(dist_nbit_)::param_type param{min_bound, max_bound};
        dist_nbit_.param(param);
    }

  private:
    const int deterministic_seed_{13596};  /**< seed of random generator */
    std::mt19937 gen_;                     /**< random integer generator */
    uniform_int_distribution<> dist_nbit_; /**< rule of generator */
};

}  // namespace svt_av1_test_tool
/** @} */  // end of svt_av1_test_tool

#endif  // _TEST_RANDOM_H_
