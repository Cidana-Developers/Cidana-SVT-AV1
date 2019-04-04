/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file InvTxfm1dTest.cc
 *
 * @brief Unit test for inverse 1d transform functions:
 * - av1_idct{4, 8, 16, 32, 64}_new
 * - av1_iadst{4, 8, 16}_new
 * - av1_iidentity{4, 8, 16, 32}_new
 *
 * @author Cidana-Edmond, Cidana-Wenyao
 *
 ******************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <new>

#include "random.h"
#include "util.h"
#include "gtest/gtest.h"

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

#include "TxfmCommon.h"

using svt_av1_test_tool::SVTRandom;
using svt_av1_test_tool::round_shift;
namespace {

using InvTxfm1dParam = std::tuple<TXFM_TYPE, int>;
/**
 * @brief Unit test for inverse 1d tx functions:
 * - av1_idct{4, 8, 16, 32, 64}_new
 * - av1_iadst{4, 8, 16}_new
 * - av1_iidentity{4, 8, 16, 32}_new
 *
 * Test strategy:
 * Verify by running forward transform and inverse transform in pairs
 * and check the max error between input and inv_output, which
 * should be smaller than 2;
 *
 * Expected result:
 * The difference should be smaller than the max_error, which is specified
 * by algorithm analysis.
 *
 * Test coverage:
 * The input to this kind of function should dequant coeffs or intermedia
 * data in 2d fwd functions.
 *
 * Test cases:
 * - C/AV1InvTxfm1dTest.run_inv_accuracy_check
 */
class AV1InvTxfm1dTest : public ::testing::TestWithParam<InvTxfm1dParam> {
  public:
    AV1InvTxfm1dTest()
        : txfm_type_(TEST_GET_PARAM(0)), max_error_(TEST_GET_PARAM(1)) {
        txfm_size_ = get_txfm1d_size(txfm_type_);
    }

    void run_inv_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < txfm_size_; ++ni) {
                input_[ni] = rnd.random_10s();
                output_[ni] = 0;
                inv_output_[ni] = 255;  // setup different output
            }

            const int inv_cos_bit = INV_COS_BIT;
            const int fwd_cos_bit = inv_cos_bit;
            fwd_txfm_type_to_func(txfm_type_)(
                input_, output_, fwd_cos_bit, test_txfm_range);
            // calculate in inverse transform functions
            inv_txfm_type_to_func(txfm_type_)(
                output_, inv_output_, inv_cos_bit, test_txfm_range);

            // compare betwenn input and inversed output
            for (int ni = 0; ni < txfm_size_; ++ni) {
                EXPECT_LE(
                    abs(input_[ni] -
                        round_shift(inv_output_[ni], get_msb(txfm_size_) - 1)),
                    max_error_)
                    << "inv txfm type " << txfm_type_ << " size " << txfm_size_
                    << " loop: " << ti;
            }
        }
    }

  private:
    const int max_error_;       /**< max error allowed */
    int txfm_size_;             /**< transform size, max transform is DCT64 */
    const TXFM_TYPE txfm_type_; /**< tx type, including dct, iadst, idtx */
    static const int max_txfm_size_ = 64;
    DECLARE_ALIGNED(32, int32_t, input_[max_txfm_size_]);
    DECLARE_ALIGNED(32, int32_t, output_[max_txfm_size_]);
    DECLARE_ALIGNED(32, int32_t, inv_output_[max_txfm_size_]);
};

TEST_P(AV1InvTxfm1dTest, run_inv_accuracy_check) {
    run_inv_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1InvTxfm1dTest,
    ::testing::Values(
        InvTxfm1dParam(TXFM_TYPE_DCT4, 2), InvTxfm1dParam(TXFM_TYPE_DCT8, 2),
        InvTxfm1dParam(TXFM_TYPE_DCT16, 2), InvTxfm1dParam(TXFM_TYPE_DCT32, 2),
        InvTxfm1dParam(TXFM_TYPE_DCT64, 2), InvTxfm1dParam(TXFM_TYPE_ADST4, 2),
        InvTxfm1dParam(TXFM_TYPE_ADST8, 2), InvTxfm1dParam(TXFM_TYPE_ADST16, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY4, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY8, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY16, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY32, 2)));
}  // namespace
