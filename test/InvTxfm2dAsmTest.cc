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
#include <algorithm>
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
#include "av1_inv_txfm_ssse3.h"

namespace {

using InvSqrTxfm2dFun = void (*)(const int32_t *input, uint16_t *output,
                                 int32_t stride, TxType tx_type, int32_t bd);
using InvRectTxfm2dType1Func = void (*)(const int32_t *input, uint16_t *output,
                                        int32_t stride, TxType tx_type,
                                        TxSize tx_size, int32_t eob,
                                        int32_t bd);
using InvRectTxfm2dType2Func = void (*)(const int32_t *input, uint16_t *output,
                                        int32_t stride, TxType tx_type,
                                        TxSize tx_size, int32_t bd);
typedef struct {
    const char *name;
    InvSqrTxfm2dFun ref_func;
    InvSqrTxfm2dFun test_func;
    IsTxTypeImpFunc check_imp_func;
} InvSqrTxfmFuncPair;

typedef struct {
    InvRectTxfm2dType2Func ref_func;
    InvRectTxfm2dType2Func test_func;
} InvRectType2TxfmFuncPair;

#define SQR_FUNC_PAIRS(name, type, is_tx_type_imp)                \
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
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_4x4, avx2, all_txtype_imp),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_8x8, avx2, all_txtype_imp),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_16x16, avx2, all_txtype_imp),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_32x32, avx2, is_tx_type_imp_32x32_avx2),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_64x64),
};

static const InvSqrTxfmFuncPair inv_txfm_c_sse4_1_func_pairs[TX_64X64 + 1] = {
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_4x4, sse4_1, dct_adst_combine_imp),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_8x8, sse4_1, dct_adst_combine_imp),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_16x16, sse4_1, dct_adst_combine_imp),
    EMPTY_FUNC_PAIRS(av1_inv_txfm2d_add_32x32),
    SQR_FUNC_PAIRS(av1_inv_txfm2d_add_64x64, sse4_1, is_tx_type_imp_64x64_sse4),
};

// from TX_4X8 to TX_SIZES_ALL
static const InvRectTxfm2dType1Func rect_type1_ref_funcs[TX_SIZES_ALL] = {
    // square transform
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,  // 4x8 and 8x4
    av1_inv_txfm2d_add_8x16_c,
    av1_inv_txfm2d_add_16x8_c,
    av1_inv_txfm2d_add_16x32_c,
    av1_inv_txfm2d_add_32x16_c,
    av1_inv_txfm2d_add_32x64_c,
    av1_inv_txfm2d_add_64x32_c,
    nullptr,
    nullptr,  // 4x16 and 16x4
    av1_inv_txfm2d_add_8x32_c,
    av1_inv_txfm2d_add_32x8_c,
    av1_inv_txfm2d_add_16x64_c,
    av1_inv_txfm2d_add_64x16_c};

static const InvRectType2TxfmFuncPair inv_4x8{av1_inv_txfm2d_add_4x8_c,
                                              av1_inv_txfm2d_add_4x8_sse4_1};
static const InvRectType2TxfmFuncPair inv_8x4{av1_inv_txfm2d_add_8x4_c,
                                              av1_inv_txfm2d_add_8x4_sse4_1};
static const InvRectType2TxfmFuncPair inv_4x16{av1_inv_txfm2d_add_4x16_c,
                                               av1_inv_txfm2d_add_4x16_sse4_1};
static const InvRectType2TxfmFuncPair inv_16x4{av1_inv_txfm2d_add_16x4_c,
                                               av1_inv_txfm2d_add_16x4_sse4_1};
static const InvRectType2TxfmFuncPair *get_rect_type2_func_pair(
    const TxSize tx_size) {
    switch (tx_size) {
    case TX_4X8: return &inv_4x8;
    case TX_8X4: return &inv_8x4;
    case TX_4X16: return &inv_4x16;
    case TX_16X4: return &inv_16x4;
    default: return nullptr;
    }
}

const int deterministic_seed = 0xa42b;
/**
 * @brief Unit test for inverse tx 2d avx2/sse4_1 functions:
 * - av1_inv_txfm2d_{4, 8, 16, 32, 64}x{4, 8, 16, 32, 64}_avx2
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check test output and reference output. Four tests
 * are required since there are three different function signatures and one
 * set of function for lowbd functions.
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
class InvTxfm2dAsmTest : public ::testing::TestWithParam<int> {
  public:
    InvTxfm2dAsmTest() : bd_(GetParam()), gen_(deterministic_seed) {
        decltype(u_nbit_)::param_type u_param{0, (1 << bd_) - 1};
        u_nbit_.param(u_param);
        decltype(s_nbit_)::param_type s_param{-(1 << bd_) + 1, (1 << bd_) - 1};
        s_nbit_.param(s_param);
    }

    ~InvTxfm2dAsmTest() {
        aom_clear_system_state();
    }

    void run_sqr_txfm_match_test(const TxSize tx_size, int asm_type) {
        const int width = get_txb_wide(tx_size);
        const int height = get_txb_high(tx_size);
        InvSqrTxfmFuncPair pair = (asm_type == 0)
                                      ? inv_txfm_c_avx2_func_pairs[tx_size]
                                      : inv_txfm_c_sse4_1_func_pairs[tx_size];
        if (pair.ref_func == nullptr || pair.test_func == nullptr)
            return;
        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);
            const IsTxTypeImpFunc is_tx_type_imp = pair.check_imp_func;

            // tx_type and tx_size are not compatible in the av1-spec.
            // like the max size of adst transform is 16, and max size of
            // identity transform is 32.
            if (is_txfm_allowed(type, width, height) == false)
                continue;

            // Some tx_type is not implemented yet, so we will skip this;
            if (is_tx_type_imp(type) == false)
                continue;

            const int loops = 100;
            for (int k = 0; k < loops; k++) {
                populate_with_random(width, height, type, tx_size);

                pair.ref_func(input_, output_ref_, stride_, type, bd_);
                pair.test_func(input_, output_test_, stride_, type, bd_);

                EXPECT_EQ(0,
                          memcmp(output_ref_,
                                 output_test_,
                                 height * stride_ * sizeof(output_test_[0])))
                    << "loop: " << k << " tx_type: " << tx_type
                    << " tx_size: " << tx_size << " asm_type: " << asm_type;
            }
        }
    }

    void run_rect_type1_txfm_match_test(const TxSize tx_size) {
        const int width = get_txb_wide(tx_size);
        const int height = get_txb_high(tx_size);
        const int max_eob = av1_get_max_eob(tx_size);

        const InvRectTxfm2dType1Func test_func = av1_highbd_inv_txfm_add_avx2;
        const InvRectTxfm2dType1Func ref_func = rect_type1_ref_funcs[tx_size];
        if (ref_func == nullptr)
            return;

        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);

            // tx_type and tx_size are not compatible in the av1-spec.
            // like the max size of adst transform is 16, and max size of
            // identity transform is 32.
            if (is_txfm_allowed(type, width, height) == false)
                continue;

            // Some tx_type is implemented but will crash the unit test.
            // Skip this kind of test and fail it instead.
            if (is_rect_type1_crash_case(tx_size, type)) {
                FAIL();
                continue;
            }

            const int loops = 100;
            for (int k = 0; k < loops; k++) {
                populate_with_random(width, height, type, tx_size);

                // Since input_ is fixed, we can not iterator the eob
                // from 1 to max_eob, otherwise all the coeffs are cleared
                // when eob is equal to 1;
                for (int eob = max_eob; eob > 0; --eob) {
                    clear_high_freq_coeffs(tx_size, type, eob, max_eob);

                    ref_func(
                        input_, output_ref_, stride_, type, tx_size, eob, bd_);
                    test_func(
                        input_, output_test_, stride_, type, tx_size, eob, bd_);
                }

                ASSERT_EQ(0,
                          memcmp(output_ref_,
                                 output_test_,
                                 height * stride_ * sizeof(output_test_[0])))
                    << "loop: " << k << " tx_type: " << tx_type
                    << " tx_size: " << tx_size;
            }
        }
    }

    void run_rect_type2_txfm_match_test(const TxSize tx_size) {
        const int width = get_txb_wide(tx_size);
        const int height = get_txb_high(tx_size);
        const InvRectType2TxfmFuncPair *test_pair =
            get_rect_type2_func_pair(tx_size);
        if (test_pair == nullptr)
            return;

        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);

            // tx_type and tx_size are not compatible in the av1-spec.
            // like the max size of adst transform is 16, and max size of
            // identity transform is 32.
            if (is_txfm_allowed(type, width, height) == false)
                continue;

            const int loops = 100;
            for (int k = 0; k < loops; k++) {
                populate_with_random(width, height, type, tx_size);

                test_pair->ref_func(
                    input_, output_ref_, stride_, type, tx_size, bd_);
                test_pair->test_func(
                    input_, output_test_, stride_, type, tx_size, bd_);

                ASSERT_EQ(0,
                          memcmp(output_ref_,
                                 output_test_,
                                 height * stride_ * sizeof(output_test_[0])))
                    << "loop: " << k << " tx_type: " << tx_type
                    << " tx_size: " << tx_size;
            }
        }
    }

    void run_lowbd_txfm_match_test(const TxSize tx_size) {
        if (bd_ > 8)
            return;
        const int width = get_txb_wide(tx_size);
        const int height = get_txb_high(tx_size);
        const int max_eob = av1_get_max_eob(tx_size);
        using LowbdInvRectTxfmRefFunc = void (*)(const int32_t *input,
                                                 uint16_t *output,
                                                 int32_t stride,
                                                 TxType tx_type,
                                                 TxSize tx_size,
                                                 int32_t eob,
                                                 int32_t bd);
        using LowbdInvSqrTxfmRefFunc = void (*)(const int32_t *input,
                                                uint16_t *output,
                                                int32_t stride,
                                                TxType tx_type,
                                                int32_t bd);
        const LowbdInvSqrTxfmRefFunc lowbd_sqr_ref_funcs[TX_SIZES] = {
            av1_inv_txfm2d_add_4x4_c,
            av1_inv_txfm2d_add_8x8_c,
            av1_inv_txfm2d_add_16x16_c,
            av1_inv_txfm2d_add_32x32_c,
            av1_inv_txfm2d_add_64x64_c};
        const LowbdInvRectTxfmRefFunc lowbd_rect_ref_funcs[TX_SIZES_ALL] = {
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            av1_inv_txfm2d_add_8x16_c,
            av1_inv_txfm2d_add_16x8_c,
            av1_inv_txfm2d_add_16x32_c,
            av1_inv_txfm2d_add_32x16_c,
            av1_inv_txfm2d_add_32x64_c,
            av1_inv_txfm2d_add_64x32_c,
            nullptr,
            nullptr,
            av1_inv_txfm2d_add_8x32_c,
            av1_inv_txfm2d_add_32x8_c,
            av1_inv_txfm2d_add_16x64_c,
            av1_inv_txfm2d_add_64x16_c};

        if (tx_size >= TX_SIZES && lowbd_rect_ref_funcs[tx_size] == nullptr)
            return;

        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            TxType type = static_cast<TxType>(tx_type);

            // tx_type and tx_size are not compatible in the av1-spec.
            // like the max size of adst transform is 16, and max size of
            // identity transform is 32.
            if (is_txfm_allowed(type, width, height) == false)
                continue;

            const int loops = 100;
            for (int k = 0; k < loops; k++) {
                populate_with_random(width, height, type, tx_size);

                // copy to lowbd output buffer from short buffer
                DECLARE_ALIGNED(32, uint8_t, lowbd_output_test_[MAX_TX_SQUARE]);
                for (int i = 0; i < height; i++) {
                    for (int j = 0; j < width; j++)
                        lowbd_output_test_[i * stride_ + j] =
                            static_cast<uint8_t>(output_test_[i * stride_ + j]);
                }

                for (int eob = max_eob; eob > 0; --eob) {
                    clear_high_freq_coeffs(tx_size, type, eob, max_eob);
                    av1_lowbd_inv_txfm2d_add_ssse3(input_,
                                                   lowbd_output_test_,
                                                   stride_,
                                                   type,
                                                   tx_size,
                                                   eob);
                    if (tx_size >= TX_SIZES)
                        lowbd_rect_ref_funcs[tx_size](input_,
                                                      output_ref_,
                                                      stride_,
                                                      type,
                                                      tx_size,
                                                      eob,
                                                      bd_);
                    else
                        lowbd_sqr_ref_funcs[tx_size](
                            input_, output_ref_, stride_, type, bd_);
                }
                for (int i = 0; i < height; i++) {
                    for (int j = 0; j < width; j++) {
                        uint8_t ref =
                            static_cast<uint8_t>(output_ref_[i * stride_ + j]);
                        ASSERT_EQ(ref, lowbd_output_test_[i * stride_ + j])
                            << "loop: " << k << " tx_type: " << tx_type
                            << " tx_size: " << tx_size;
                    }
                }
            }
        }
    }

  private:
    // TODO(wenyao): update these cases as these issues are fixed.
    bool is_rect_type1_crash_case(const TxSize tx_size, const TxType tx_type) {
        std::vector<TxType> invalid_types;
        switch (tx_size) {
        case TX_16X32:
            invalid_types = std::vector<TxType>{H_DCT, H_ADST, H_FLIPADST};
            break;
        case TX_32X64:
        case TX_64X32:
            invalid_types = std::vector<TxType>{IDTX, V_DCT, H_DCT};
            break;
        case TX_16X64:
            invalid_types =
                std::vector<TxType>{IDTX, V_DCT, H_DCT, H_ADST, H_FLIPADST};
            break;
        case TX_64X16:
            invalid_types =
                std::vector<TxType>{IDTX, V_DCT, H_DCT, V_ADST, V_FLIPADST};
            break;
        default: break;
        }

        auto it =
            std::find(invalid_types.begin(), invalid_types.end(), tx_type);
        return it == invalid_types.end() ? false : true;
    }

    void clear_high_freq_coeffs(const TxSize tx_size, const TxType tx_type,
                                const int eob, const int max_eob) {
        const SCAN_ORDER *scan_order = &av1_scan_orders[tx_size][tx_type];
        const int16_t *scan = scan_order->scan;

        // clear the coeffs between eob and max_eob, note the input stride has
        // to be respected.
        for (int i = eob; i < max_eob; ++i) {
            const int pos = scan[i];
            // handle large transform;
            const int32_t bwl = get_txb_bwl(tx_size);
            const int32_t row = pos >> bwl;
            const int32_t col = pos - (row << bwl);
            input_[row * stride_ + col] = 0;
        }
    }

    void populate_with_random(const int width, const int height,
                              const TxType tx_type, const TxSize tx_size) {
        using FwdTxfm2dFunc = void (*)(int16_t * input,
                                       int32_t * output,
                                       uint32_t stride,
                                       TxType tx_type,
                                       uint8_t bd);

        const FwdTxfm2dFunc fwd_txfm_func[TX_SIZES_ALL] = {
            Av1TransformTwoD_4x4_c,   Av1TransformTwoD_8x8_c,
            Av1TransformTwoD_16x16_c, Av1TransformTwoD_32x32_c,
            Av1TransformTwoD_64x64_c, av1_fwd_txfm2d_4x8_c,
            av1_fwd_txfm2d_8x4_c,     av1_fwd_txfm2d_8x16_c,
            av1_fwd_txfm2d_16x8_c,    av1_fwd_txfm2d_16x32_c,
            av1_fwd_txfm2d_32x16_c,   av1_fwd_txfm2d_32x64_c,
            av1_fwd_txfm2d_64x32_c,   av1_fwd_txfm2d_4x16_c,
            av1_fwd_txfm2d_16x4_c,    av1_fwd_txfm2d_8x32_c,
            av1_fwd_txfm2d_32x8_c,    av1_fwd_txfm2d_16x64_c,
            av1_fwd_txfm2d_64x16_c,
        };

        memset(output_ref_, 0, sizeof(output_ref_));
        memset(output_test_, 0, sizeof(output_test_));
        memset(input_, 0, sizeof(input_));
        memset(tmp_input_, 0, sizeof(tmp_input_));
        memset(pixel_input_, 0, sizeof(pixel_input_));
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                pixel_input_[i * stride_ + j] = s_nbit_(gen_);
                output_ref_[i * stride_ + j] = output_test_[i * stride_ + j] =
                    u_nbit_(gen_);
            }
        }

        fwd_txfm_func[tx_size](pixel_input_, tmp_input_, stride_, tx_type, bd_);
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                input_[i * stride_ + j] = tmp_input_[i * width + j];
            }
        }

        return;
    }

  private:
    std::mt19937 gen_; /**< seed for random */
    std::uniform_int_distribution<> u_nbit_;
    std::uniform_int_distribution<> s_nbit_;

    const int bd_; /**< input param 8bit or 10bit */
    static const int stride_ = MAX_TX_SIZE;
    DECLARE_ALIGNED(32, int16_t, pixel_input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, int32_t, input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, int32_t, tmp_input_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, uint16_t, output_ref_[MAX_TX_SQUARE]);
};

TEST_P(InvTxfm2dAsmTest, sqr_txfm_match_test) {
    for (int asm_type = 0; asm_type < 2; ++asm_type) {
        for (int i = TX_4X4; i <= TX_64X64; i++) {
            const TxSize tx_size = static_cast<TxSize>(i);
            run_sqr_txfm_match_test(tx_size, asm_type);
        }
    }
}

TEST_P(InvTxfm2dAsmTest, rect_type1_txfm_match_test) {
    for (int i = TX_4X8; i < TX_SIZES_ALL; i++) {
        const TxSize tx_size = static_cast<TxSize>(i);
        run_rect_type1_txfm_match_test(tx_size);
    }
}

TEST_P(InvTxfm2dAsmTest, rect_type2_txfm_match_test) {
    for (int i = TX_4X8; i < TX_SIZES_ALL; i++) {
        const TxSize tx_size = static_cast<TxSize>(i);
        run_rect_type2_txfm_match_test(tx_size);
    }
}

TEST_P(InvTxfm2dAsmTest, lowbd_txfm_match_test) {
    for (int i = TX_4X4; i < TX_SIZES_ALL; i++) {
        const TxSize tx_size = static_cast<TxSize>(i);
        run_lowbd_txfm_match_test(tx_size);
    }
}

INSTANTIATE_TEST_CASE_P(TX, InvTxfm2dAsmTest,
                        ::testing::Values(static_cast<int>(AOM_BITS_8),
                                          static_cast<int>(AOM_BITS_10)));
}  // namespace
