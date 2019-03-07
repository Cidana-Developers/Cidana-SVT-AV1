/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#include "gtest/gtest.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <new>

#include "random.h"
#include "util.h"

#include "EbDefinitions.h"
#include "EbTransforms.h"

#include "TxfmTest.h"

using svt_av1_test_tool::SVTRandom;

namespace {
void reference_dct_1d(const double *in, double *out, int size) {
    const double kInvSqrt2 = 0.707106781186547524400844362104;
    for (int k = 0; k < size; ++k) {
        out[k] = 0;
        for (int n = 0; n < size; ++n) {
            out[k] += in[n] * cos(PI * (2 * n + 1) * k / (2 * size));
        }
        if (k == 0)
            out[k] = out[k] * kInvSqrt2;
    }
}

void reference_adst_1d(const double *in, double *out, int size) {
    for (int k = 0; k < size; ++k) {
        out[k] = 0;
        for (int n = 0; n < size; ++n) {
            out[k] += in[n] * sin(PI * (2 * n + 1) * (2 * k + 1) / (4 * size));
        }
    }
}

// TODO: find fidentity refernce method

typedef void (*Txfm1dFuncRef)(const double *in, double *out, int size);

class TrfmTestBase {
  public:
    virtual ~TrfmTestBase() {
    }

  protected:
    void run_fwd_accuracy_check() {
        SVTRandom rnd;
        const int count_test_block = 5000;
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < _txfm_size; ++ni) {
                _input[ni] = rnd.random_8() - rnd.random_8();
                _input_ref[ni] = static_cast<double>(_input[ni]);
            }

            // calculate in fdct functions
            fwd_txfm_type_to_func(_fwd_txfm_type)(
                _input, _output, 13, fwd_txfm_range_mult2_list[_fwd_txfm_type]);
            // calculate in reference fdct functions
            if (_fwd_txfm_ref)
                _fwd_txfm_ref(_input_ref, _output_ref, _txfm_size);

            // compare for the result is in accuracy
            for (int ni = 0; ni < _txfm_size; ++ni) {
                ASSERT_LE(abs(_output[ni] -
                              static_cast<int32_t>(round(_output_ref[ni]))),
                          _max_error);
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
};

typedef std::tr1::tuple<TXFM_TYPE, Txfm1dFuncRef, int, int> Trfm1dParam;

class AV1FwdTxfm : public TrfmTestBase,
                   public ::testing::TestWithParam<Trfm1dParam> {
  public:
    virtual void SetUp() {
        _fwd_txfm_type = TEST_GET_PARAM(0);
        _fwd_txfm_ref = TEST_GET_PARAM(1);
        _txfm_size = TEST_GET_PARAM(2);
        _max_error = TEST_GET_PARAM(3);
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

TEST_P(AV1FwdTxfm, RunFwdAccuracyCheck) {
    run_fwd_accuracy_check();
}

INSTANTIATE_TEST_CASE_P(
    C, AV1FwdTxfm,
    ::testing::Values(Trfm1dParam(TXFM_TYPE_DCT4, &reference_dct_1d, 4, 1),
                      Trfm1dParam(TXFM_TYPE_DCT8, &reference_dct_1d, 8, 1),
                      Trfm1dParam(TXFM_TYPE_DCT16, &reference_dct_1d, 16, 2),
                      Trfm1dParam(TXFM_TYPE_DCT32, &reference_dct_1d, 32, 3),
                      Trfm1dParam(TXFM_TYPE_ADST4, &reference_adst_1d, 4, 1),
                      Trfm1dParam(TXFM_TYPE_ADST8, &reference_adst_1d, 8, 1),
                      Trfm1dParam(TXFM_TYPE_ADST16, &reference_adst_1d, 16, 2),
                      Trfm1dParam(TXFM_TYPE_ADST32, &reference_adst_1d, 32, 3),
                      Trfm1dParam(TXFM_TYPE_IDENTITY4, NULL, 4, 1),
                      Trfm1dParam(TXFM_TYPE_IDENTITY8, NULL, 8, 1),
                      Trfm1dParam(TXFM_TYPE_IDENTITY16, NULL, 16, 2),
                      Trfm1dParam(TXFM_TYPE_IDENTITY32, NULL, 32, 3)));
}  // namespace
