/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file EncodeTxbAsmTest.cc
 *
 * @brief Unit test for av1_txb_init_levels_avx2:
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <random>

#include "gtest/gtest.h"

// Workaround to eliminate the compiling warning on linux
// The macro will conflict with definition in gtest.h
#ifdef __USE_GNU
#undef __USE_GNU  // defined in EbThreads.h
#endif
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE  // defined in EbThreads.h
#endif

#include "EbDefinitions.h"
#include "EbTransforms.h"
#include "util.h"
#include "aom_dsp_rtcd.h"

namespace EncodeTxbAsmTest {

const int deterministic_seed = 0xa42b;
static INLINE int32_t get_txb_wide(TxSize tx_size) {
    tx_size = av1_get_adjusted_tx_size(tx_size);
    return tx_size_wide[tx_size];
}
static INLINE int32_t get_txb_high(TxSize tx_size) {
    tx_size = av1_get_adjusted_tx_size(tx_size);
    return tx_size_high[tx_size];
}
static INLINE uint8_t *set_levels(uint8_t *const levels_buf,
                                  const int32_t width) {
    return levels_buf + TX_PAD_TOP * (width + TX_PAD_HOR);
}

// test assembly code of av1_txb_init_levels
using TxbInitLevelsFunc = void (*)(const tran_low_t *const coeff,
                                   const int width, const int height,
                                   uint8_t *const levels);
using TxbInitLevelParam = std::tuple<TxbInitLevelsFunc, int>;
/**
 * @brief Unit test for av1_txb_init_levels_avx2:
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check the difference between test output
 * and reference output.
 *
 * Expect result:
 * Output from assemble function should be exactly same as output from c.
 *
 * Test coverage:
 * Input buffer: Fill with random values
 * width: deduced from valid tx_size
 * height: deduced from valid tx_size
 *
 */
class EncodeTxbInitLevelTest
    : public ::testing::TestWithParam<TxbInitLevelParam> {
  public:
    EncodeTxbInitLevelTest()
        : gen_(deterministic_seed), ref_func_(&av1_txb_init_levels_c) {
        std::uniform_int_distribution<> uni_dist(0, UINT8_MAX);
        for (int i = 0; i < TX_PAD_2D; i++) {
            levels_buf_test_[i] = uni_dist(gen_);
            levels_buf_ref_[i] = uni_dist(gen_);
        }
    }

    virtual ~EncodeTxbInitLevelTest() {
        aom_clear_system_state();
    }

    void check_txb_init_levels_assembly(TxbInitLevelsFunc test_func,
                                        int tx_size) {
        const int width = get_txb_wide((TxSize)tx_size);
        const int height = get_txb_high((TxSize)tx_size);

        levels_test_ = set_levels(levels_buf_test_, width);
        levels_ref_ = set_levels(levels_buf_ref_, width);

        std::uniform_int_distribution<> uni_dist(0, UINT16_MAX);
        for (int i = 0; i < width * height; i++) {
            input_coeff_[i] = uni_dist(gen_) - INT16_MAX;
        }

        ref_func_(input_coeff_, width, height, levels_ref_);
        test_func(input_coeff_, width, height, levels_test_);

        const int stride = width + TX_PAD_HOR;
        for (int r = 0; r < height + TX_PAD_VER; ++r) {
            for (int c = 0; c < stride; ++c) {
                ASSERT_EQ(levels_buf_test_[c + r * stride],
                          levels_buf_ref_[c + r * stride])
                    << "[" << r << "," << c << "] " << width << "x" << height;
            }
        }
    }

  private:
    std::mt19937 gen_;
    uint8_t levels_buf_test_[TX_PAD_2D];
    uint8_t levels_buf_ref_[TX_PAD_2D];
    tran_low_t input_coeff_[MAX_TX_SQUARE];

    uint8_t *levels_test_;
    uint8_t *levels_ref_;
    const TxbInitLevelsFunc ref_func_;
};

TEST_P(EncodeTxbInitLevelTest, txb_init_levels_assmbly) {
    check_txb_init_levels_assembly(TEST_GET_PARAM(0), TEST_GET_PARAM(1));
}

INSTANTIATE_TEST_CASE_P(
    AVX2, EncodeTxbInitLevelTest,
    ::testing::Combine(::testing::Values(&av1_txb_init_levels_avx2),
                       ::testing::Range(0, static_cast<int>(TX_SIZES_ALL), 1)));
}  // namespace EncodeTxbAsmTest
