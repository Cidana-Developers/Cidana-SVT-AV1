/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file InvTxfm2dAsmTest.c
 *
 * @brief Unit test for forward 2d transform functions:
 * - Av1TransformTwoD_{4x4, 8x8, 16x16, 32x32, 64x64}
 * - av1_fwd_txfm2d_{rectangle}
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/
#include "gtest/gtest.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
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
#include "TxfmCommon.h"

namespace {

using InvTxfm2dAsmParam = std::tuple<int, int, int>;
using InvTxfm2dFunc = void (*)(const int32_t *input, uint16_t *output,
                               int32_t stride, TxType tx_type, int32_t bd);
typedef struct {
    const char *name;
    InvTxfm2dFunc ref_func;
    InvTxfm2dFunc test_func;
} InvTxfmFuncPair;

#define FUNC_PAIRS(name, type)                                 \
    {                                                          \
        #name, reinterpret_cast < InvTxfm2dFunc > (name##_c),  \
            reinterpret_cast < InvTxfm2dFunc > (name##_##type) \
    }

#define EMPTY_FUNC_PAIRS(name) \
    { #name, nullptr, nullptr }

#define FIXED_TARGET_PAIRS(name, target)                      \
    {                                                         \
        #name, reinterpret_cast < InvTxfm2dFunc > (name##_c), \
            reinterpret_cast < InvTxfm2dFunc > (target)       \
    }

static const InvTxfmFuncPair inv_txfm_c_avx2_func_pairs[TX_SIZES_ALL] = {
    FUNC_PAIRS(av1_inv_txfm2d_add_4x4, avx2),
    FUNC_PAIRS(av1_inv_txfm2d_add_8x8, avx2),
    FUNC_PAIRS(av1_inv_txfm2d_add_16x16, avx2),
    FUNC_PAIRS(av1_inv_txfm2d_add_32x32, avx2),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_64x64),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_4x8),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_8x4),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_8x16, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_16x8, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_16x32, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_32x16, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_32x64, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_64x32, av1_highbd_inv_txfm_add_avx2),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_4x16),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_16x4),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_8x32, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_32x8, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_16x64, av1_highbd_inv_txfm_add_avx2),
    FIXED_TARGET_PAIRS(av1_inv_txfm2d_add_64x16, av1_highbd_inv_txfm_add_avx2),
};

static const InvTxfmFuncPair inv_txfm_c_sse4_1_func_pairs[TX_SIZES_ALL] = {
    FUNC_PAIRS(av1_inv_txfm2d_add_4x4, sse4_1),
    FUNC_PAIRS(av1_inv_txfm2d_add_8x8, sse4_1),
    FUNC_PAIRS(av1_inv_txfm2d_add_16x16, sse4_1),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x32),
    FUNC_PAIRS(av1_inv_txfm2d_add_64x64, sse4_1),
    FUNC_PAIRS(av1_inv_txfm2d_add_4x8, sse4_1),
    FUNC_PAIRS(av1_inv_txfm2d_add_8x4, sse4_1),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_8x16),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_16x8),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_16x32),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x16),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x64),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_64x32),
    FUNC_PAIRS(av1_inv_txfm2d_add_4x16, sse4_1),
    FUNC_PAIRS(av1_inv_txfm2d_add_16x4, sse4_1),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_8x32),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x8),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_16x64),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_64x16),
};

const int deterministic_seed = 0xa42b;
/**
 * @brief Unit test for inverse tx 2d avx2/sse4_1 functions:
 * - av1_inv_txfm2d_{4, 8, 16, 32, 64}x{4, 8, 16, 32, 64}_avx2
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check test output and reference output.
 *
 * Expect result:
 * Output from assemble function should be exactly same as output from c.
 *
 * Test coverage:
 * Test cases:
 * Input buffer: Fill with random values
 * TxSize: all the valid TxSize and TxType allowed.
 * BitDepth: 8bit and 10bit
 * AssembleType: avx2 and sse4_1
 *
 */
class InvTxfm2dAsmTest : public ::testing::TestWithParam<InvTxfm2dAsmParam> {
  public:
    InvTxfm2dAsmTest()
        : tx_size_(static_cast<TxSize>(TEST_GET_PARAM(0))),
          bd_(TEST_GET_PARAM(1)),
          asm_type_(TEST_GET_PARAM(2)),
          gen_(deterministic_seed) {
        decltype(input_dist_nbit_)::param_type param{
            -(1 << (bd_ + 7)), (1 << (bd_ + 7)) - 1};  // input is the dequant
                                                       // values, the range is
                                                       // set according to
                                                       // spec 7.12.3
        input_dist_nbit_.param(param);
        width_ = get_txb_wide(tx_size_);
        height_ = get_txb_high(tx_size_);
    }

    ~InvTxfm2dAsmTest() {
        aom_clear_system_state();
    }

    void run_match_test() {
        InvTxfmFuncPair pair = (asm_type_ == 0)
                                   ? inv_txfm_c_avx2_func_pairs[tx_size_]
                                   : inv_txfm_c_sse4_1_func_pairs[tx_size_];
        if (pair.ref_func == nullptr || pair.test_func == nullptr)
            return;
        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);
            if (is_txfm_valid(type, width_, height_) == false)
                continue;

            const int loops = 1;
            for (int k = 0; k < loops; k++) {
                populate_with_random();
                std::cout << "tx_size: " << tx_size_ << " tx_type: " << type
                          << std::endl;
                pair.ref_func(input_, output_ref_, stride, type, bd_);
                pair.test_func(input_, output_test_, stride, type, bd_);

                for (int i = 0; i < height_; i++)
                    for (int j = 0; j < width_; j++) {
                        ASSERT_EQ(output_ref_[i * width_ + j],
                                  output_test_[i * width_ + j])
                            << "loop: " << k << " tx_type: " << tx_type
                            << " tx_size: " << tx_size_ << " Mismatch at (" << j
                            << " x " << i << ")";
                    }
            }
        }
    }

  private:
    void populate_with_random() {
        std::uniform_int_distribution<> dist_nbit;
        decltype(dist_nbit)::param_type param{0, (1 << bd_) - 1};
        dist_nbit.param(param);

        for (int i = 0; i < height_; i++) {
            for (int j = 0; j < width_; j++) {
                input_[i * stride + j] = input_dist_nbit_(gen_);
                output_ref_[i * width_ + j] = output_test_[i * width_ + j] =
                    dist_nbit(gen_);
            }
        }

        return;
    }

  private:
    std::mt19937 gen_;                                /**< seed for random */
    std::uniform_int_distribution<> input_dist_nbit_; /**< generate random for
                                                         input buffer for 8bit
                                                         and 10bit coeffs */
    const TxSize tx_size_; /**< input param tx_size */
    const int bd_;         /**< input param 8bit or 10bit */
    const int asm_type_;   /**< 0: avx2, 1: sse4_1 */
    int width_;
    int height_;
    static const int stride = MAX_TX_SIZE;
    DECLARE_ALIGNED(32, int32_t, input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_ref_[MAX_TX_SQUARE]);
};

TEST_P(InvTxfm2dAsmTest, match_test) {
    run_match_test();
}

INSTANTIATE_TEST_CASE_P(
    ASM, InvTxfm2dAsmTest,
    ::testing::Combine(::testing::Range(static_cast<int>(TX_4X4),
                                        static_cast<int>(TX_SIZES_ALL), 1),
                       ::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10)),
                       ::testing::Values(0, 1)));
}  // namespace
