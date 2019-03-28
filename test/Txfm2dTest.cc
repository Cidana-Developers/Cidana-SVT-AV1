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
#include "reference.h"
#include "util.h"

#include "TxfmTest.h"

using svt_av1_test_tool::SVTRandom;
using svt_av1_test_reference::data_amplify;
using svt_av1_test_reference::reference_txfm_2d;

namespace {

typedef std::tuple<TxSize, TxType, int> FwdTxfm2dParam;

class AV1FwdTxfm2dTest : public ::testing::TestWithParam<FwdTxfm2dParam> {
  public:
    AV1FwdTxfm2dTest() {
        txfm_size_ = TEST_GET_PARAM(0);
        txfm_type_ = TEST_GET_PARAM(1);
        max_error_ = TEST_GET_PARAM(2);
        Av1TransformConfig(txfm_type_, txfm_size_, &cfg_);
        const int block_size =
            tx_size_wide[cfg_.tx_size] * tx_size_high[cfg_.tx_size];

        input_test_ = new int16_t[block_size];
        output_test_ = new int32_t[block_size];
        input_ref_ = new double[block_size];
        output_ref_ = new double[block_size];
        // make output different by default.
        memset(output_ref_, 0, sizeof(double) * block_size);
        memset(output_test_, 128, sizeof(int32_t) * block_size);
    }

    ~AV1FwdTxfm2dTest() {
        delete[] input_test_;
        delete[] output_test_;
        delete[] input_ref_;
        delete[] output_ref_;
    }

  protected:
    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 1000;
        const int block_size = tx_size_wide[cfg_.tx_size] * tx_size_high[cfg_.tx_size];
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < block_size; ++ni) {
                input_test_[ni] = rnd.random_10();
                input_ref_[ni] = static_cast<double>(input_test_[ni]);
            }

            // calculate in forward transform functions
            fwd_txfm_2d_size_to_func(txfm_size_)(input_test_,
                                                 output_test_,
                                                 tx_size_wide[cfg_.tx_size],
                                                 txfm_type_,
                                                 14);

            // calculate in reference forward transform functions
            fwd_txfm_2d_reference(input_ref_, output_ref_);

            // compare for the result is in accuracy
            double test_max_error = 0;
            for (int ni = 0; ni < block_size; ++ni) {
                test_max_error =
                    max(test_max_error,
                        fabs(output_test_[ni] - round(output_ref_[ni])));
            }
            ASSERT_GE(max_error_, test_max_error)
                << "fwd txfm 2d test tx_type: " << txfm_type_
                << " tx_size: " << txfm_size_ << " loop: " << ti;
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
        flip_input(input);
        reference_txfm_2d(input, output, txfm_type_, txfm_size_);
        data_amplify(output_ref_,
                     txfm_size_,
                     cfg_.shift[0] + cfg_.shift[1] + cfg_.shift[2]);
    }

  private:
    double max_error_;
    TxSize txfm_size_;
    TxType txfm_type_;
    TXFM_2D_FLIP_CFG cfg_;
    int16_t *input_test_;
    int32_t *output_test_;
    double *input_ref_;
    double *output_ref_;
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

INSTANTIATE_TEST_CASE_P(C, AV1FwdTxfm2dTest,
                        ::testing::ValuesIn(gen_txfm_2d_params()));

}  // namespace
