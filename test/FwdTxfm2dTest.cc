/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file FwdTxfm2dTest.c
 *
 * @brief Unit test for forward 2d transform functions:
 * - Av1TransformTwoD_{4x4, 8x8, 16x16, 32x32, 64x64}
 * - av1_fwd_txfm2d_{rectangle}
 *
 * @author Cidana-Edmond, Cidana-Wenyao
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
#include "TxfmRef.h"
#include "util.h"

#include "TxfmCommon.h"

using svt_av1_test_reference::data_amplify;
using svt_av1_test_reference::reference_txfm_2d;
using svt_av1_test_tool::SVTRandom;

namespace {

using FwdTxfm2dParam = std::tuple<TxSize, TxType, int>;
/**
 * @brief Unit test for forward 2d tx functions:
 * - Av1TransformTwoD_{4x4, 8x8, 16x16, 32x32, 64x64}
 * - av1_fwd_txfm2d_{rectangle}
 *
 * Test strategy:
 * Verify these tx function by comparing with reference implementation.
 * Feed the same data and check the difference between test output
 * and reference output.
 *
 * Expected result:
 * The difference should be smaller than the max_error, which is specified
 * by algorithm analysis.
 *
 * Test coverage:
 * The input to this kind of function should be residual.
 *
 * Test cases:
 * - C/AV1FwdTxfm2dTest.run_fwd_accuracy_check
 */
class AV1FwdTxfm2dTest : public ::testing::TestWithParam<FwdTxfm2dParam> {
  public:
    AV1FwdTxfm2dTest()
        : txfm_size_(TEST_GET_PARAM(0)),
          txfm_type_(TEST_GET_PARAM(1)),
          max_error_(TEST_GET_PARAM(2)) {
        Av1TransformConfig(txfm_type_, txfm_size_, &cfg_);
    }

  protected:
    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 1000;
        const int block_size =
            tx_size_wide[cfg_.tx_size] * tx_size_high[cfg_.tx_size];
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < block_size; ++ni) {
                input_test_[ni] = rnd.random_10();
                input_ref_[ni] = static_cast<double>(input_test_[ni]);
                output_ref_[ni] = 0;
                output_test_[ni] = 255;
            }

            // calculate in forward transform functions
            fwd_txfm_2d_size_to_func(txfm_size_)(input_test_,
                                                 output_test_,
                                                 tx_size_wide[cfg_.tx_size],
                                                 txfm_type_,
                                                 8);

            // calculate in reference forward transform functions
            fwd_txfm_2d_reference(input_ref_, output_ref_);

            // compare for the result is in accuracy
            double test_max_error = 0;
            for (int ni = 0; ni < block_size; ++ni) {
                test_max_error =
                    max(test_max_error,
                        fabs(output_test_[ni] - round(output_ref_[ni])));
                ASSERT_GE(max_error_, test_max_error)
                    << "fwd txfm 2d test tx_type: " << txfm_type_
                    << " tx_size: " << txfm_size_ << " loop: " << ti;
            }
        }
    }

  private:
    void flip_input(double *input) {
        int width = tx_size_wide[cfg_.tx_size];
        int height = tx_size_high[cfg_.tx_size];
        if (cfg_.lr_flip) {
            for (int r = 0; r < height; ++r) {
                for (int c = 0; c < width / 2; ++c) {
                    const double tmp = input[r * width + c];
                    input[r * width + c] = input[r * width + width - 1 - c];
                    input[r * width + width - 1 - c] = tmp;
                }
            }
        }
        if (cfg_.ud_flip) {
            for (int c = 0; c < width; ++c) {
                for (int r = 0; r < height / 2; ++r) {
                    const double tmp = input[r * width + c];
                    input[r * width + c] = input[(height - 1 - r) * width + c];
                    input[(height - 1 - r) * width + c] = tmp;
                }
            }
        }
    }

    void fwd_txfm_2d_reference(double *input, double *output) {
        const int block_size =
            tx_size_wide[cfg_.tx_size] * tx_size_high[cfg_.tx_size];

        flip_input(input);
        reference_txfm_2d(input, output, txfm_type_, txfm_size_);
        data_amplify(output_ref_,
                     block_size,
                     cfg_.shift[0] + cfg_.shift[1] + cfg_.shift[2]);
    }

  private:
    const double max_error_;
    const TxSize txfm_size_;
    const TxType txfm_type_;
    Txfm2DFlipCfg cfg_;
    DECLARE_ALIGNED(32, int16_t, input_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, int32_t, output_test_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, double, input_ref_[MAX_TX_SQUARE]);
    DECLARE_ALIGNED(32, double, output_ref_[MAX_TX_SQUARE]);
};

TEST_P(AV1FwdTxfm2dTest, run_fwd_accuracy_check) {
    run_fwd_accuracy_check();
}

static double max_error_ls[TX_SIZES_ALL] = {
    8,    // 4x4 transform
    9,    // 8x8 transform
    12,   // 16x16 transform
    70,   // 32x32 transform
    64,   // 64x64 transform
    4,    // 4x8 transform
    5,    // 8x4 transform
    12,   // 8x16 transform
    12,   // 16x8 transform
    32,   // 16x32 transform
    46,   // 32x16 transform
    136,  // 32x64 transform
    136,  // 64x32 transform
    5,    // 4x16 transform
    6,    // 16x4 transform
    21,   // 8x32 transform
    13,   // 32x8 transform
    30,   // 16x64 transform
    36,   // 64x16 transform
};

static std::vector<FwdTxfm2dParam> gen_txfm_2d_params() {
    std::vector<FwdTxfm2dParam> param_vec;
    for (int s = 0; s < TX_SIZES; ++s) {
        const double max_error = max_error_ls[s];
        for (int t = 0; t < TX_TYPES; ++t) {
            const TxType txfm_type = static_cast<TxType>(t);
            const TxSize txfm_size = static_cast<TxSize>(s);
            if (valid_txsize_txtype(txfm_size, txfm_type)) {
                param_vec.push_back(
                    FwdTxfm2dParam(txfm_size, txfm_type, max_error));
            }
        }
    }
    return param_vec;
}

INSTANTIATE_TEST_CASE_P(TX, AV1FwdTxfm2dTest,
                        ::testing::ValuesIn(gen_txfm_2d_params()));

}  // namespace
