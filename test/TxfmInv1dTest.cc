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

namespace {
class TxfmTestBase {
  public:
    virtual ~TxfmTestBase() {
    }

  protected:
    void run_inv_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        const int8_t inv_txfm_range[12] = {
            20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < _txfm_size; ++ni) {
                _input[ni] =
                    (rnd.random_16() & 0x03FF) - (rnd.random_16() & 0x03FF);
            }

            // calculate in forward transform functions
            fwd_txfm_type_to_func(_txfm_type)(
                _input, _output, 14, fwd_txfm_range_mult2_list[_txfm_type]);
            // calculate in inverse transform functions
            inv_txfm_type_to_func(_txfm_type)(
                _output, _inv_output, 14, inv_txfm_range);

            // compare betwenn input and inversed output
            for (int ni = 0; ni < _txfm_size; ++ni) {
                EXPECT_LE(
                    abs(_input[ni] -
                        svt_av1_test_tool::round_shift(
                            _inv_output[ni],get_msb(_txfm_size) - 1)),
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
    int32_t *_inv_output;
    std::string _err_str;
};

typedef std::tr1::tuple<TXFM_TYPE, int, int, std::string> TxfmInv1dParam;

class AV1InvTxfmTest : public TxfmTestBase,
                       public ::testing::TestWithParam<TxfmInv1dParam> {
  public:
    virtual void SetUp() {
        _txfm_type = TEST_GET_PARAM(0);
        _txfm_size = TEST_GET_PARAM(1);
        _max_error = TEST_GET_PARAM(2);
        _err_str = TEST_GET_PARAM(3);
        if (_txfm_size > 0) {
            _input = new int32_t[_txfm_size];
            memset(_input, 0, sizeof(int32_t) * _txfm_size);
            _output = new int32_t[_txfm_size];
            memset(_output, 0, sizeof(int32_t) * _txfm_size);
            _inv_output = new int32_t[_txfm_size];
            memset(_inv_output, 0, sizeof(int32_t) * _txfm_size);
        }
    }
    virtual void TearDown() {
        if (_input)
            delete[] _input;
        if (_output)
            delete[] _output;
        if (_inv_output)
            delete[] _inv_output;
    }
};

TEST_P(AV1InvTxfmTest, RunInvAccuracyCheck) {
    run_inv_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1InvTxfmTest,
    ::testing::Values(
        TxfmInv1dParam(TXFM_TYPE_DCT4, 4, 2, "inverse DCT4 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_DCT8, 8, 2, "inverse DCT8 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_DCT16, 16, 2, "inverse DCT16 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_DCT32, 32, 2, "inverse DCT32 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_ADST4, 4, 2, "inverse ADST4 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_ADST8, 8, 2, "inverse ADST8 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_ADST16, 16, 2, "inverse ADST16 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_ADST32, 32, 2, "inverse ADST32 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_IDENTITY4, 4, 2, "inverse IDTX4 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_IDENTITY8, 8, 2, "inverse IDTX8 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_IDENTITY16, 16, 2,
                       "inverse IDTX16 test failed!"),
        TxfmInv1dParam(TXFM_TYPE_IDENTITY32, 32, 2,
                       "inverse IDTX32 test failed!")));
}  // namespace
