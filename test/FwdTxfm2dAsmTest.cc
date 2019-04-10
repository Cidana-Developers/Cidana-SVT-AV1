/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file FwdTxfm2dAsmTest.c
 *
 * @brief Unit test for forward 2d transform functions written in assembly code:
 * - av1_fwd_txfm2d_{4, 8, 16, 32, 64}x{4, 8, 16, 32, 64}_avx2
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/
#include "gtest/gtest.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>

// workaround to eliminate the compiling warning on linux
// The macro will conflict with definition in gtest.h
#ifdef __USE_GNU
#undef __USE_GNU  // defined in EbThreads.h
#endif
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE  // defined in EbThreads.h
#endif
#include "EbDefinitions.h"
#include "EbTransforms.h"
#include "random.h"
#include "util.h"
#include "aom_dsp_rtcd.h"
#include "EbTransforms.h"

using svt_av1_test_tool::SVTRandom;
/**
 * @brief Unit test for fwd tx 2d avx2 functions:
 * - av1_fwd_txfm2d_{4, 8, 16, 32, 64}x{4, 8, 16, 32, 64}_avx2
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check test output and reference output.
 * The test output and reference output are different at the beginning.
 *
 * Expect result:
 * Output from assemble function should be exactly same as output from c.
 *
 * Test coverage:
 * Test cases:
 * Input buffer: Fill with random values
 * TxSize: all the valid TxSize and TxType allowed.
 * BitDepth: 8bit and 10bit.
 *
 */
namespace {
using FwdTxfm2dAsmParam = std::tuple<int, int>;
using FwdTxfm2dFunc = void (*)(int16_t *input, int32_t *output, uint32_t stride,
                               TxType tx_type, uint8_t bd);
typedef struct {
    const char *name;
    FwdTxfm2dFunc ref_func;
    FwdTxfm2dFunc test_func;
} TxfmFuncPair;

#define FUNC_PAIRS(name, type)                                 \
    {                                                          \
        #name, reinterpret_cast < FwdTxfm2dFunc > (name##_c),  \
            reinterpret_cast < FwdTxfm2dFunc > (name##_##type) \
    }

static const TxfmFuncPair txfm_func_pairs[TX_SIZES_ALL] = {
    {"av1_fwd_txfm2d_4x4", Av1TransformTwoD_4x4_c, av1_fwd_txfm2d_4x4_sse4_1},
    {"av1_fwd_txfm2d_8x8", Av1TransformTwoD_8x8_c, av1_fwd_txfm2d_8x8_avx2},
    {"av1_fwd_txfm2d_16x16",
     Av1TransformTwoD_16x16_c,
     av1_fwd_txfm2d_16x16_avx2},
    {"av1_fwd_txfm2d_32x32",
     Av1TransformTwoD_32x32_c,
     av1_fwd_txfm2d_32x32_avx2},
    {"av1_fwd_txfm2d_64x64",
     Av1TransformTwoD_64x64_c,
     av1_fwd_txfm2d_64x64_avx2},
    FUNC_PAIRS(av1_fwd_txfm2d_4x8, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_8x4, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_8x16, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_16x8, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_16x32, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_32x16, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_32x64, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_64x32, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_4x16, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_16x4, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_8x32, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_32x8, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_16x64, avx2),
    FUNC_PAIRS(av1_fwd_txfm2d_64x16, avx2),
};

static INLINE int32_t get_txb_wide(TxSize tx_size) {
    tx_size = av1_get_adjusted_tx_size(tx_size);
    return tx_size_wide[tx_size];
}

static INLINE int32_t get_txb_high(TxSize tx_size) {
    tx_size = av1_get_adjusted_tx_size(tx_size);
    return tx_size_high[tx_size];
}

const int deterministic_seed = 0xa42b;
class FwdTxfm2dAsmTest : public ::testing::TestWithParam<FwdTxfm2dAsmParam> {
  public:
    FwdTxfm2dAsmTest()
        : tx_size_(static_cast<TxSize>(TEST_GET_PARAM(0))),
          bd_(TEST_GET_PARAM(1)),
          gen_(deterministic_seed) {
        decltype(dist_nbit_)::param_type param{-(1 << bd_) + 1, (1 << bd_) - 1};
        dist_nbit_.param(param);
        width = get_txb_wide(tx_size_);
        height = get_txb_high(tx_size_);
        memset(output_test_, 0, sizeof(output_test_));
        memset(output_ref_, 255, sizeof(output_ref_));  // -1
    }

    ~FwdTxfm2dAsmTest() {
        aom_clear_system_state();
    }

    void run_match_test() {
        TxfmFuncPair pair = txfm_func_pairs[tx_size_];
        std::cout << pair.name << std::endl;
        if (pair.ref_func == nullptr || pair.test_func == nullptr)
            return;
        for (int tx_type = 0; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);
            if (is_txfm_valid(type) == false)
                continue;

            const int loops = 10;
            for (int k = 0; k < loops; k++) {
                populate_with_random();

                pair.ref_func(input_, output_ref_, stride, type, bd_);
                pair.test_func(input_, output_test_, stride, type, bd_);

                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++)
                        ASSERT_EQ(output_ref_[i * width + j],
                                  output_test_[i * width + j])
                            << "loop: " << k << " tx_type: " << tx_type
                            << " tx_size: " << tx_size_ << " Mismatch at (" << j
                            << " x " << i << ")";
            }
        }
    }

  private:
    void populate_with_random() {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                input_[i * stride + j] = dist_nbit_(gen_);
            }
        }

        return;
    }

    bool is_txfm_valid(TxType tx_type) {
        const TX_TYPE_1D vert_type = vtx_tab[tx_type];
        const TX_TYPE_1D horz_type = htx_tab[tx_type];
        const int max_size[TX_TYPES_1D] = {64, 16, 16, 32};
        if (width <= max_size[horz_type] && height <= max_size[vert_type])
            return true;
        else
            return false;
    }

  private:
    std::mt19937 gen_; /**< seed for random */
    std::uniform_int_distribution<>
        dist_nbit_;        /**< random int for 8bit and 10bit coeffs */
    const TxSize tx_size_; /**< input param tx_size */
    const int bd_;         /**< input param 8bit or 10bit */
    int width;
    int height;
    static const int stride = MAX_TX_SIZE;
    DECLARE_ALIGNED(32, int16_t, input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, int32_t, output_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, int32_t, output_ref_[MAX_TX_SQUARE]);
};

TEST_P(FwdTxfm2dAsmTest, match_test) {
    run_match_test();
}

INSTANTIATE_TEST_CASE_P(
    AVX2, FwdTxfm2dAsmTest,
    ::testing::Combine(::testing::Range(static_cast<int>(TX_4X4),
                                        static_cast<int>(TX_SIZES_ALL), 1),
                       ::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10))));
}  // namespace
