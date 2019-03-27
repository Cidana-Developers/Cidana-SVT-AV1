/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

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
#include "EncodeTxbRef.h"
#include "util.h"

namespace EncodeTxbAsmTest {
// test assembly code of av1_get_nz_map_contexts
// TODO(wenyao): av1_get_nz_map_contexts_c is not availbale, we should discuss
// this with netflix.
const int deterministic_seed = 0xa42b;
extern "C" void av1_get_nz_map_contexts_sse2(const uint8_t *const levels,
                                             const int16_t *const scan,
                                             const uint16_t eob, TxSize tx_size,
                                             const TX_CLASS tx_class,
                                             int8_t *const coeff_contexts);
using GetNzMapContextsFunc = void (*)(const uint8_t *const levels,
                                      const int16_t *const scan,
                                      const uint16_t eob, const TxSize tx_size,
                                      const TX_CLASS tx_class,
                                      int8_t *const coeff_contexts);
using GetNzMapContextParam = std::tuple<GetNzMapContextsFunc, int, int>;

class EncodeTxbTest : public ::testing::TestWithParam<GetNzMapContextParam> {
  public:
    EncodeTxbTest()
        : gen_(deterministic_seed), ref_func_(&av1_get_nz_map_contexts_c) {
    }

    virtual ~EncodeTxbTest() {
        aom_clear_system_state();
    }

    void check_get_nz_map_context_assembly(GetNzMapContextsFunc test_func,
                                           const int tx_type,
                                           const int tx_size) {
        const int num_tests = 10;
        const TX_CLASS tx_class = tx_type_to_class[tx_type];

        const int bwl = get_txb_bwl((TxSize)tx_size);
        const int width = get_txb_wide((TxSize)tx_size);
        const int height = get_txb_high((TxSize)tx_size);
        const int real_width = tx_size_wide[tx_size];
        const int real_height = tx_size_high[tx_size];
        const int16_t *const scan = av1_scan_orders[tx_size][tx_type].scan;

        levels_ = set_levels(levels_buf_, width);
        for (int i = 0; i < num_tests; ++i) {
            for (int eob = 1; eob <= width * height; ++eob) {
                init_levels(scan, bwl, eob);
                init_coeff_contexts(scan, bwl, eob);

                ref_func_(levels_,
                          scan,
                          eob,
                          (TxSize)tx_size,
                          tx_class,
                          coeff_contexts_ref_);
                test_func(levels_,
                          scan,
                          eob,
                          (TxSize)tx_size,
                          tx_class,
                          coeff_contexts_test_);

                for (int j = 0; j < eob; ++j) {
                    const int pos = scan[j];
                    ASSERT_EQ(coeff_contexts_ref_[pos],
                              coeff_contexts_test_[pos])
                        << " tx_class " << tx_class << " width " << real_width
                        << " height " << real_height << " eob " << eob;
                }
            }
        }
    }

  private:
    void init_levels(const int16_t *const scan, const int bwl, const int eob) {
        std::uniform_int_distribution<> uni_dist(0, INT8_MAX);
        memset(levels_buf_, 0, sizeof(levels_buf_));
        for (int c = 0; c < eob; ++c) {
            levels_[get_padded_idx(scan[c], bwl)] =
                static_cast<uint8_t>(uni_dist(gen_));
        }
    }

    void init_coeff_contexts(const int16_t *const scan, const int bwl,
                             const int eob) {
        std::uniform_int_distribution<> uni_dist(0, UINT8_MAX);
        memset(coeff_contexts_test_,
               0,
               sizeof(*coeff_contexts_test_) * MAX_TX_SQUARE);
        memset(coeff_contexts_ref_,
               0,
               sizeof(*coeff_contexts_ref_) * MAX_TX_SQUARE);
        // Generate oppsite value for these two buffers
        for (int c = 0; c < eob; ++c) {
            const int pos = scan[c];
            coeff_contexts_test_[pos] = uni_dist(gen_) - INT8_MAX;
            coeff_contexts_ref_[pos] = -coeff_contexts_test_[pos];
        }
    }

    uint8_t levels_buf_[TX_PAD_2D];
    uint8_t *levels_;
    DECLARE_ALIGNED(16, int8_t, coeff_contexts_ref_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(16, int8_t, coeff_contexts_test_[MAX_TX_SQUARE]);
    std::mt19937 gen_;
    const GetNzMapContextsFunc ref_func_;
};

TEST_P(EncodeTxbTest, get_nz_map_context_assembly) {
    check_get_nz_map_context_assembly(
        TEST_GET_PARAM(0), TEST_GET_PARAM(1), TEST_GET_PARAM(2));
}

INSTANTIATE_TEST_CASE_P(
    SSE2, EncodeTxbTest,
    ::testing::Combine(::testing::Values(&av1_get_nz_map_contexts_sse2),
                       ::testing::Range(0, static_cast<int>(TX_TYPES), 1),
                       ::testing::Range(0, static_cast<int>(TX_SIZES_ALL), 1)));

// test assembly code of av1_txb_init_levels
extern "C" void av1_txb_init_levels_avx2(const tran_low_t *const coeff,
                                         const int32_t width,
                                         const int32_t height,
                                         uint8_t *const levels);
// defined in EbRateDistortionCost.c
extern "C" void av1_txb_init_levels_c(const tran_low_t *const coeff,
                                      const int32_t width, const int32_t height,
                                      uint8_t *const levels);

using TxbInitLevelsFunc = void (*)(const tran_low_t *const coeff,
                                   const int width, const int height,
                                   uint8_t *const levels);
using TxbInitLevelParam = std::tuple<TxbInitLevelsFunc, int>;

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
        tran_low_t coeff[MAX_TX_SQUARE];

        levels_test_ = set_levels(levels_buf_test_, width);
        levels_ref_ = set_levels(levels_buf_ref_, width);

        std::uniform_int_distribution<> uni_dist(0, UINT16_MAX);
        for (int i = 0; i < width * height; i++) {
            coeff[i] = uni_dist(gen_) - INT16_MAX;
        }

        ref_func_(coeff, width, height, levels_ref_);
        test_func(coeff, width, height, levels_test_);

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
