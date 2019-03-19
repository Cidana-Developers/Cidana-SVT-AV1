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

namespace {

typedef std::tuple<TXFM_TYPE, int, std::string> TxfmFwd1dParam;

class AV1FwdTxfmTest : public ::testing::TestWithParam<TxfmFwd1dParam> {
  public:
    AV1FwdTxfmTest() {
        _txfm_type = TEST_GET_PARAM(0);
        _txfm_size = get_txfm1d_size(_txfm_type);
        _max_error = TEST_GET_PARAM(1);
        _err_str = TEST_GET_PARAM(2);
        if (_txfm_size > 0) {
            _input = new int32_t[_txfm_size];
            memset(_input, 0, sizeof(int32_t) * _txfm_size);
            _output = new int32_t[_txfm_size];
            memset(_output, 0, sizeof(int32_t) * _txfm_size);
            _input_ref = new double[_txfm_size];
            memset(_input_ref, 0, sizeof(double) * _txfm_size);
            _output_ref = new double[_txfm_size];
            memset(_output_ref, 0, sizeof(double) * _txfm_size);
        }
    }
    virtual ~AV1FwdTxfmTest() {
        if (_input)
            delete[] _input;
        if (_output)
            delete[] _output;
        if (_input_ref)
            delete[] _input_ref;
        if (_output_ref)
            delete[] _output_ref;
    }

  protected:
    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < _txfm_size; ++ni) {
                _input[ni] = rnd.random_10s();
                _input_ref[ni] = static_cast<double>(_input[ni]);
            }

            // calculate in forward transform functions
            fwd_txfm_type_to_func(_txfm_type)(
                _input, _output, 14, test_txfm_range);
            // calculate in reference forward transform functions
            svt_av1_test_reference::reference_txfm_1d(
                svt_av1_test_reference::get_txfm1d_types(_txfm_type),
                _input_ref,
                _output_ref,
                _txfm_size);

            // compare for the result is in accuracy
            for (int ni = 0; ni < _txfm_size; ++ni) {
                ASSERT_LE(abs(_output[ni] -
                              static_cast<int32_t>(round(_output_ref[ni]))),
                          _max_error)
                    << _err_str;
            }
        }
    }

    double _max_error;
    int _txfm_size;
    TXFM_TYPE _txfm_type;
    int32_t *_input;
    int32_t *_output;
    double *_input_ref;
    double *_output_ref;
    std::string _err_str;
};

TEST_P(AV1FwdTxfmTest, run_fwd_accuracy_check) {
    run_fwd_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1FwdTxfmTest,
    ::testing::Values(
        TxfmFwd1dParam(TXFM_TYPE_DCT4, 7, "av1_fdct4_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_DCT8, 7, "av1_fdct8_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_DCT16, 7, "av1_fdct16_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_DCT32, 7, "av1_fdct32_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_ADST4, 7, "av1_fadst4_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_ADST8, 7, "av1_fadst8_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_ADST16, 7, "av1_fadst16_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_ADST32, 7, "av1_fadst32_new test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_IDENTITY4, 7, "av1_fidentity4_c test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_IDENTITY8, 7, "av1_fidentity8_c test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_IDENTITY16, 7,
                       "av1_fidentity16_c test failed!"),
        TxfmFwd1dParam(TXFM_TYPE_IDENTITY32, 7,
                       "av1_fidentity32_c test failed!")));

}  // namespace
