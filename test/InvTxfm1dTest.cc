#include "gtest/gtest.h"

#include <math.h>
#include <stdlib.h>
#include <new>

#include "random.h"
#include "util.h"

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

#include "TxfmTest.h"

using svt_av1_test_tool::SVTRandom;
using svt_av1_test_tool::round_shift;
namespace {

typedef std::tuple<TXFM_TYPE, int> InvTxfm1dParam;

class AV1InvTxfm1dTest : public ::testing::TestWithParam<InvTxfm1dParam> {
  public:
    AV1InvTxfm1dTest() {
        txfm_type_ = TEST_GET_PARAM(0);
        max_error_ = TEST_GET_PARAM(1);
        txfm_size_ = get_txfm1d_size(txfm_type_);

        input_ = new int32_t[txfm_size_];
        memset(input_, 0, sizeof(int32_t) * txfm_size_);
        output_ = new int32_t[txfm_size_];
        memset(output_, 0, sizeof(int32_t) * txfm_size_);
        inv_output_ = new int32_t[txfm_size_];
        memset(inv_output_, 0, sizeof(int32_t) * txfm_size_);
    }

    virtual ~AV1InvTxfm1dTest() {
        delete[] input_;
        delete[] output_;
        delete[] inv_output_;
    }

    void run_inv_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < txfm_size_; ++ni) {
                input_[ni] = rnd.random_10s();
            }

            // calculate in forward transform functions
            fwd_txfm_type_to_func(txfm_type_)(
                input_, output_, 14, test_txfm_range);
            // calculate in inverse transform functions
            inv_txfm_type_to_func(txfm_type_)(
                output_, inv_output_, 14, test_txfm_range);

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
    double max_error_;
    int txfm_size_;
    TXFM_TYPE txfm_type_;
    int32_t *input_;
    int32_t *output_;
    int32_t *inv_output_;
};

TEST_P(AV1InvTxfm1dTest, run_inv_accuracy_check) {
    run_inv_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1InvTxfm1dTest,
    ::testing::Values(
        InvTxfm1dParam(TXFM_TYPE_DCT4, 2), InvTxfm1dParam(TXFM_TYPE_DCT8, 2),
        InvTxfm1dParam(TXFM_TYPE_DCT16, 2), InvTxfm1dParam(TXFM_TYPE_DCT32, 2),
        InvTxfm1dParam(TXFM_TYPE_ADST4, 2), InvTxfm1dParam(TXFM_TYPE_ADST8, 2),
        InvTxfm1dParam(TXFM_TYPE_ADST16, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY4, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY8, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY16, 2),
        InvTxfm1dParam(TXFM_TYPE_IDENTITY32, 2)));
}  // namespace
