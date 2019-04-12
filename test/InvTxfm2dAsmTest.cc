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

using InvTxfm2dAsmParam = std::tuple<int, int>;
using InvSqrTxfm2dFun = void (*)(const int32_t *input, uint16_t *output,
                                 int32_t stride, TxType tx_type, int32_t bd);
typedef struct {
    const char *name;
    InvSqrTxfm2dFun ref_func;
    InvSqrTxfm2dFun test_func;
    IsTxTypeImpFunc check_imp_func;
} InvSqrTxfmFuncPair;

#define FUNC_PAIRS(name, type, is_tx_type_imp)                    \
    {                                                             \
        #name, reinterpret_cast < InvSqrTxfm2dFun > (name##_c),   \
            reinterpret_cast < InvSqrTxfm2dFun > (name##_##type), \
            is_tx_type_imp                                        \
    }

#define EMPTY_FUNC_PAIRS(name) \
    { #name, nullptr, nullptr, nullptr }

static bool is_tx_type_imp_32x32_avx2(const TxType tx_type) {
    switch (tx_type) {
    case DCT_DCT:
    case IDTX: return true;
    default: return false;
    }
}

static bool is_tx_type_imp_64x64_sse4(const TxType tx_type) {
    if (tx_type == DCT_DCT)
        return true;
    return false;
}

static const InvSqrTxfmFuncPair inv_txfm_c_avx2_func_pairs[TX_64X64 + 1] = {
    FUNC_PAIRS(av1_inv_txfm2d_add_4x4, avx2, all_txtype_imp),
    FUNC_PAIRS(av1_inv_txfm2d_add_8x8, avx2, all_txtype_imp),
    FUNC_PAIRS(av1_inv_txfm2d_add_16x16, avx2, all_txtype_imp),
    FUNC_PAIRS(av1_inv_txfm2d_add_32x32, avx2, is_tx_type_imp_32x32_avx2),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_64x64),
};

static const InvSqrTxfmFuncPair inv_txfm_c_sse4_1_func_pairs[TX_64X64 + 1] = {
    FUNC_PAIRS(av1_inv_txfm2d_add_4x4, sse4_1, dct_adst_combine_imp),
    FUNC_PAIRS(av1_inv_txfm2d_add_8x8, sse4_1, dct_adst_combine_imp),
    FUNC_PAIRS(av1_inv_txfm2d_add_16x16, sse4_1, dct_adst_combine_imp),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x32),
    FUNC_PAIRS(av1_inv_txfm2d_add_64x64, sse4_1, is_tx_type_imp_64x64_sse4),
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
        : bd_(TEST_GET_PARAM(0)),
          asm_type_(TEST_GET_PARAM(1)),
          gen_(deterministic_seed) {
        // input is the dequant values, the range is set according to
        // spec 7.12.3
        decltype(input_dist_nbit_)::param_type param{-(1 << (bd_ + 7)),
                                                     (1 << (bd_ + 7)) - 1};
        input_dist_nbit_.param(param);
    }

    ~InvTxfm2dAsmTest() {
        aom_clear_system_state();
    }

    void run_sqr_txfm_match_test(const TxSize tx_size) {
        const int width = get_txb_wide(tx_size);
        const int height = get_txb_high(tx_size);
        InvSqrTxfmFuncPair pair = (asm_type_ == 0)
                                      ? inv_txfm_c_avx2_func_pairs[tx_size]
                                      : inv_txfm_c_sse4_1_func_pairs[tx_size];
        if (pair.ref_func == nullptr || pair.test_func == nullptr)
            return;
        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);
            const IsTxTypeImpFunc is_tx_type_imp = pair.check_imp_func;

            if (is_txfm_allowed(type, width, height) == false)
                continue;

            if (is_tx_type_imp(type) == false)
                continue;

            const int loops = 1;
            for (int k = 0; k < loops; k++) {
                populate_with_random(width, height);

                pair.ref_func(input_, output_ref_, stride_, type, bd_);
                pair.test_func(input_, output_test_, stride_, type, bd_);

                EXPECT_EQ(0,
                          memcmp(output_ref_,
                                 output_test_,
                                 height * stride_ * sizeof(output_test_[0])))
                    << "loop: " << k << " tx_type: " << tx_type
                    << " tx_size: " << tx_size << " asm_type: " << asm_type_;
            }
        }
    }

  private:
    void populate_with_random(const int width, const int height) {
        std::uniform_int_distribution<> dist_nbit;
        decltype(dist_nbit)::param_type param{0, (1 << bd_) - 1};
        dist_nbit.param(param);

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                input_[i * stride_ + j] = input_dist_nbit_(gen_);
                output_ref_[i * stride_ + j] = output_test_[i * stride_ + j] =
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
    const int bd_;       /**< input param 8bit or 10bit */
    const int asm_type_; /**< 0: avx2, 1: sse4_1 */
    static const int stride_ = MAX_TX_SIZE;
    DECLARE_ALIGNED(32, int32_t, input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_ref_[MAX_TX_SQUARE]);
};

TEST_P(InvTxfm2dAsmTest, sqr_txfm_match_test) {
    for (int i = TX_4X4; i <= TX_64X64; i++) {
        const TxSize tx_size = static_cast<TxSize>(i);
        run_sqr_txfm_match_test(tx_size);
    }
}

INSTANTIATE_TEST_CASE_P(
    ASM, InvTxfm2dAsmTest,
    ::testing::Combine(::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10)),
                       ::testing::Values(0, 1)));
}  // namespace
