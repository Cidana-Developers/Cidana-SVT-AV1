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

typedef std::tuple<TxSize, TxType, int, std::string> TxfmFwd2dParam;

class AV1FwdTxfmTest2D : public ::testing::TestWithParam<TxfmFwd2dParam> {
  public:
    AV1FwdTxfmTest2D() {
        _txfm_size = TEST_GET_PARAM(0);
        _txfm_type = TEST_GET_PARAM(1);
        _max_error = TEST_GET_PARAM(2);
        _err_str = TEST_GET_PARAM(3);
        Av1TransformConfig(_txfm_type, _txfm_size, &_cfg);
        int txfm_size = tx_size_wide[_cfg.tx_size] * tx_size_high[_cfg.tx_size];
        if (txfm_size > 0) {
            _input = new int16_t[txfm_size];
            memset(_input, 0, sizeof(int16_t) * txfm_size);
            _output = new int32_t[txfm_size];
            memset(_output, 0, sizeof(int32_t) * txfm_size);
            _input_ref = new double[txfm_size];
            memset(_input_ref, 0, sizeof(double) * txfm_size);
            _output_ref = new double[txfm_size];
            memset(_output_ref, 0, sizeof(double) * txfm_size);
        }
    }
    ~AV1FwdTxfmTest2D() {
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
        const int count_test_block = 1000;
        int txfm_size = tx_size_wide[_cfg.tx_size] * tx_size_high[_cfg.tx_size];
        for (int ti = 0; ti < count_test_block; ++ti) {
            // prepare random test data
            for (int ni = 0; ni < txfm_size; ++ni) {
                _input[ni] = rnd.random_10();
                _input_ref[ni] = static_cast<double>(_input[ni]);
            }

            // calculate in forward transform functions
            fwd_txfm_2d_size_to_func(_txfm_size)(
                _input, _output, tx_size_wide[_cfg.tx_size], _txfm_type, 14);
            // calculate in reference forward transform functions
            fwd_txfm_2d_reference(_input_ref, _output_ref);
            svt_av1_test_reference::data_amplify(
                _output_ref,
                txfm_size,
                _cfg.shift[0] + _cfg.shift[1] + _cfg.shift[2]);

            // compare for the result is in accuracy
            double test_max_error = 0;
            for (int ni = 0; ni < txfm_size; ++ni) {
                test_max_error = max(
                    test_max_error, fabs(_output[ni] - round(_output_ref[ni])));
            }
            ASSERT_GE(_max_error, test_max_error) << _err_str;
        }
    }

  private:
    void flip_input(double *input) {
        int width = tx_size_wide[_cfg.tx_size];
        int height = tx_size_high[_cfg.tx_size];
        if (_cfg.lr_flip) {
            for (int r = 0; r < height; ++r) {
                for (int c = 0; c < width / 2; ++c) {
                    const double tmp = input[r * width + c];
                    input[r * width + c] = input[r * width + width - 1 - c];
                    input[r * width + width - 1 - c] = tmp;
                }
            }
        }
        if (_cfg.ud_flip) {
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
        svt_av1_test_reference::reference_txfm_2d(
            input, output, _txfm_type, _txfm_size);
    }

  protected:
    double _max_error;
    TxSize _txfm_size;
    TxType _txfm_type;
    TXFM_2D_FLIP_CFG _cfg;
    int16_t *_input;
    int32_t *_output;
    double *_input_ref;
    double *_output_ref;
    std::string _err_str;
};

TEST_P(AV1FwdTxfmTest2D, run_fwd_accuracy_check) {
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

static std::vector<TxfmFwd2dParam> gen_txfm_2d_params() {
    std::vector<TxfmFwd2dParam> param_vec;
    for (int s = 0; s < TX_SIZES; /*TX_SIZES_ALL;*/ ++s) {
        const double max_error = max_error_ls[s];
        for (int t = 0; t < TX_TYPES; ++t) {
            const TxType txfm_type = static_cast<TxType>(t);
            const TxSize txfm_size = static_cast<TxSize>(s);
            const std::string err_str =
                fwd_txfm_2d_type_to_name(txfm_type) + " X " +
                fwd_txfm_2d_size_to_name(txfm_size) + " is failed!";
            if (valid_txsize_txtype(txfm_size, txfm_type)) {
                param_vec.push_back(
                    TxfmFwd2dParam(txfm_size, txfm_type, max_error, err_str));
            }
        }
    }
    return param_vec;
}

INSTANTIATE_TEST_CASE_P(C, AV1FwdTxfmTest2D,
                        ::testing::ValuesIn(gen_txfm_2d_params()));

}  // namespace
