#include "gtest/gtest.h"

#include <math.h>
#include <stdint.h>
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
void reference_dct_1d(const double *in, double *out, int size) {
    const double kInvSqrt2 = 0.707106781186547524400844362104f;
    for (int k = 0; k < size; ++k) {
        out[k] = 0;
        for (int n = 0; n < size; ++n) {
            out[k] += in[n] * cos(PI * (2 * n + 1) * k / (2 * size));
        }
        if (k == 0)
            out[k] = out[k] * kInvSqrt2;
    }
}

// TODO(any): Copied from the old 'fadst4' (same as the new 'av1_fadst4_new'
// function). Should be replaced by a proper reference function that takes
// 'double' input & output.
static void fadst4_ref(const tran_low_t *input, tran_low_t *output) {
    //  16384 * sqrt(2) * sin(kPi/9) * 2 / 3
    const tran_high_t sinpi_1_9 = 5283;
    const tran_high_t sinpi_2_9 = 9929;
    const tran_high_t sinpi_3_9 = 13377;
    const tran_high_t sinpi_4_9 = 15212;

    tran_high_t x0, x1, x2, x3;
    tran_high_t s0, s1, s2, s3, s4, s5, s6, s7;
    x0 = input[0];
    x1 = input[1];
    x2 = input[2];
    x3 = input[3];

    if (!(x0 | x1 | x2 | x3)) {
        output[0] = output[1] = output[2] = output[3] = 0;
        return;
    }
    s0 = sinpi_1_9 * x0;
    s1 = sinpi_4_9 * x0;
    s2 = sinpi_2_9 * x1;
    s3 = sinpi_1_9 * x1;
    s4 = sinpi_3_9 * x2;
    s5 = sinpi_4_9 * x3;
    s6 = sinpi_2_9 * x3;
    s7 = x0 + x1 - x3;

    x0 = s0 + s2 + s5;
    x1 = sinpi_3_9 * s7;
    x2 = s1 - s3 + s6;
    x3 = s4;

    s0 = x0 + x3;
    s1 = x1;
    s2 = x2 - x3;
    s3 = x2 - x0 + x3;

    // 1-D transform scaling factor is sqrt(2).
    output[0] = (tran_low_t)svt_av1_test_tool::round_shift(s0, 14);
    output[1] = (tran_low_t)svt_av1_test_tool::round_shift(s1, 14);
    output[2] = (tran_low_t)svt_av1_test_tool::round_shift(s2, 14);
    output[3] = (tran_low_t)svt_av1_test_tool::round_shift(s3, 14);
}

void reference_adst_1d(const double *in, double *out, int size) {
    if (size == 4) {  // Special case.
        tran_low_t int_input[4];
        for (int i = 0; i < 4; ++i) {
            int_input[i] = static_cast<tran_low_t>(round(in[i]));
        }

        tran_low_t int_output[4];
        fadst4_ref(int_input, int_output);
        for (int i = 0; i < 4; ++i) {
            out[i] = int_output[i];
        }
        return;
    }
    for (int k = 0; k < size; ++k) {
        out[k] = 0;
        for (int n = 0; n < size; ++n) {
            out[k] += in[n] * sin(PI * (2 * n + 1) * (2 * k + 1) / (4 * size));
        }
    }
}

void reference_idtx_1d(const double *in, double *out, int size) {
    const double Sqrt2 = 1.4142135623730950488016887242097f;
    double scale = 0;
    switch (size) {
    case 4: scale = Sqrt2; break;
    case 8: scale = 2; break;
    case 16: scale = 2 * Sqrt2; break;
    case 32: scale = 4; break;
    default: assert(0); break;
    }

    for (int k = 0; k < size; ++k) {
        out[k] = in[k] * scale;
    }
}

typedef void (*Txfm1dFuncRef)(const double *in, double *out, int size);

class TxfmTestBase {
  public:
    virtual ~TxfmTestBase() {
    }

  protected:
    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < _txfm_size; ++ni) {
                _input[ni] =
                    (rnd.random_16() & 0x03FF) - (rnd.random_16() & 0x03FF);
                _input_ref[ni] = static_cast<double>(_input[ni]);
            }

            // calculate in forward transform functions
            fwd_txfm_type_to_func(_fwd_txfm_type)(
                _input, _output, 14, fwd_txfm_range_mult2_list[_fwd_txfm_type]);
            // calculate in reference forward transform functions
            if (_fwd_txfm_ref)
                _fwd_txfm_ref(_input_ref, _output_ref, _txfm_size);

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
    TXFM_TYPE _fwd_txfm_type;
    Txfm1dFuncRef _fwd_txfm_ref;
    int32_t *_input;
    int32_t *_output;
    double *_input_ref;
    double *_output_ref;
    std::string _err_str;
};

typedef std::tr1::tuple<TXFM_TYPE, Txfm1dFuncRef, int, int, std::string>
    Txfm1dParam;

class AV1FwdTxfmTest : public TxfmTestBase,
                       public ::testing::TestWithParam<Txfm1dParam> {
  public:
    virtual void SetUp() {
        _fwd_txfm_type = TEST_GET_PARAM(0);
        _fwd_txfm_ref = TEST_GET_PARAM(1);
        _txfm_size = TEST_GET_PARAM(2);
        _max_error = TEST_GET_PARAM(3);
        _err_str = TEST_GET_PARAM(4);
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
    virtual void TearDown() {
        if (_input)
            delete[] _input;
        if (_output)
            delete[] _output;
        if (_input_ref)
            delete[] _input_ref;
        if (_output_ref)
            delete[] _output_ref;
    }
};

TEST_P(AV1FwdTxfmTest, RunFwdAccuracyCheck) {
    run_fwd_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1FwdTxfmTest,
    ::testing::Values(Txfm1dParam(TXFM_TYPE_DCT4, &reference_dct_1d, 4, 7,
                                  "av1_fdct4_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_DCT8, &reference_dct_1d, 8, 7,
                                  "av1_fdct8_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_DCT16, &reference_dct_1d, 16, 7,
                                  "av1_fdct16_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_DCT32, &reference_dct_1d, 32, 7,
                                  "av1_fdct32_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_ADST4, &reference_adst_1d, 4, 7,
                                  "av1_fadst4_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_ADST8, &reference_adst_1d, 8, 7,
                                  "av1_fadst8_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_ADST16, &reference_adst_1d, 16, 7,
                                  "av1_fadst16_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_ADST32, &reference_adst_1d, 32, 7,
                                  "av1_fadst32_new test failed!"),
                      Txfm1dParam(TXFM_TYPE_IDENTITY4, &reference_idtx_1d, 4, 7,
                                  "av1_fidentity4_c test failed!"),
                      Txfm1dParam(TXFM_TYPE_IDENTITY8, &reference_idtx_1d, 8, 7,
                                  "av1_fidentity8_c test failed!"),
                      Txfm1dParam(TXFM_TYPE_IDENTITY16, &reference_idtx_1d, 16,
                                  7, "av1_fidentity16_c test failed!"),
                      Txfm1dParam(TXFM_TYPE_IDENTITY32, &reference_idtx_1d, 32,
                                  7, "av1_fidentity32_c test failed!")));

}  // namespace
