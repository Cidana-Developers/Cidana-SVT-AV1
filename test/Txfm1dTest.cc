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
using svt_av1_test_reference::get_txfm1d_types;
using svt_av1_test_reference::reference_txfm_1d;
namespace {

typedef std::tuple<TXFM_TYPE, int> FwdTxfm1dParam;

class AV1FwdTxfm1dTest : public ::testing::TestWithParam<FwdTxfm1dParam> {
  public:
    AV1FwdTxfm1dTest() {
        txfm_type_ = TEST_GET_PARAM(0);
        txfm_size_ = get_txfm1d_size(txfm_type_);
        max_error_ = TEST_GET_PARAM(1);
        if (txfm_size_ > 0) {
            input_test_ = new int32_t[txfm_size_];
            memset(input_test_, 0, sizeof(int32_t) * txfm_size_);
            output_test_ = new int32_t[txfm_size_];
            memset(output_test_, 0, sizeof(int32_t) * txfm_size_);
            input_ref_ = new double[txfm_size_];
            memset(input_ref_, 0, sizeof(double) * txfm_size_);
            output_ref_ = new double[txfm_size_];
            memset(output_ref_, 0, sizeof(double) * txfm_size_);
        }
    }

    virtual ~AV1FwdTxfm1dTest() {
        if (input_test_)
            delete[] input_test_;
        if (output_test_)
            delete[] output_test_;
        if (input_ref_)
            delete[] input_ref_;
        if (output_ref_)
            delete[] output_ref_;
    }

    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < txfm_size_; ++ni) {
                input_test_[ni] = rnd.random_10s();
                input_ref_[ni] = static_cast<double>(input_test_[ni]);
            }

            // calculate in forward transform functions
            fwd_txfm_type_to_func(txfm_type_)(
                input_test_, output_test_, 14, test_txfm_range);
            // calculate in reference forward transform functions
            reference_txfm_1d(get_txfm1d_types(txfm_type_),
                              input_ref_,
                              output_ref_,
                              txfm_size_);

            // compare for the result is in accuracy
            for (int ni = 0; ni < txfm_size_; ++ni) {
                ASSERT_LE(abs(output_test_[ni] -
                              static_cast<int32_t>(round(output_ref_[ni]))),
                          max_error_)
                    << "tx_size: " << txfm_size_ << "tx_type: " << txfm_type_;
            }
        }
    }

  private:
    double max_error_;
    int txfm_size_;
    TXFM_TYPE txfm_type_;
    int32_t *input_test_;
    int32_t *output_test_;
    double *input_ref_;
    double *output_ref_;
};

TEST_P(AV1FwdTxfm1dTest, run_fwd_accuracy_check) {
    run_fwd_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1FwdTxfm1dTest,
    ::testing::Values(
        FwdTxfm1dParam(TXFM_TYPE_DCT4, 7), FwdTxfm1dParam(TXFM_TYPE_DCT8, 7),
        FwdTxfm1dParam(TXFM_TYPE_DCT16, 7), FwdTxfm1dParam(TXFM_TYPE_DCT32, 7),
        FwdTxfm1dParam(TXFM_TYPE_ADST4, 7), FwdTxfm1dParam(TXFM_TYPE_ADST8, 7),
        FwdTxfm1dParam(TXFM_TYPE_ADST16, 7),
        FwdTxfm1dParam(TXFM_TYPE_IDENTITY4, 7),
        FwdTxfm1dParam(TXFM_TYPE_IDENTITY8, 7),
        FwdTxfm1dParam(TXFM_TYPE_IDENTITY16, 7),
        FwdTxfm1dParam(TXFM_TYPE_IDENTITY32, 7)));
}  // namespace
