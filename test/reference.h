
#ifndef _TEST_REFERENCE_H_
#define _TEST_REFERENCE_H_

#include <stdint.h>
#include "EbDefinitions.h"
#include "gtest/gtest.h"
#include "util.h"

namespace svt_av1_test_reference {
// forward transform 1d reference
typedef void (*Txfm1dFuncRef)(const double *in, double *out, int size);

INLINE void reference_dct_1d(const double *in, double *out, int size) {
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

INLINE void reference_adst_1d(const double *in, double *out, int size) {
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

INLINE void reference_idtx_1d(const double *in, double *out, int size) {
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

INLINE void reference_txfm_1d(TX_TYPE_1D type, const double *in, double *out,
                              int size) {
    switch (type) {
    case DCT_1D: reference_dct_1d(in, out, size); break;
    case ADST_1D:
    case FLIPADST_1D: reference_adst_1d(in, out, size); break;
    case IDTX_1D: reference_idtx_1d(in, out, size); break;
    default: assert(0); break;
    }
}

INLINE TX_TYPE_1D get_txfm1d_types(TXFM_TYPE txfm_type) {
    switch (txfm_type) {
    case TXFM_TYPE_DCT4:
    case TXFM_TYPE_DCT8:
    case TXFM_TYPE_DCT16:
    case TXFM_TYPE_DCT32:
    case TXFM_TYPE_DCT64: return DCT_1D;
    case TXFM_TYPE_ADST4:
    case TXFM_TYPE_ADST8:
    case TXFM_TYPE_ADST16:
    case TXFM_TYPE_ADST32: return ADST_1D;
    case TXFM_TYPE_IDENTITY4:
    case TXFM_TYPE_IDENTITY8:
    case TXFM_TYPE_IDENTITY16:
    case TXFM_TYPE_IDENTITY32:
    case TXFM_TYPE_IDENTITY64: return IDTX_1D;
    default: assert(0); return TX_TYPES_1D;
    }
}

INLINE void get_txfm2d_types(TxType txfm2d_type, TX_TYPE_1D *type1,
                             TX_TYPE_1D *type2) {
    switch (txfm2d_type) {
    case DCT_DCT:
        *type1 = DCT_1D;
        *type2 = DCT_1D;
        break;
    case ADST_DCT:
        *type1 = ADST_1D;
        *type2 = DCT_1D;
        break;
    case DCT_ADST:
        *type1 = DCT_1D;
        *type2 = ADST_1D;
        break;
    case ADST_ADST:
        *type1 = ADST_1D;
        *type2 = ADST_1D;
        break;
    case FLIPADST_DCT:
        *type1 = FLIPADST_1D;
        *type2 = DCT_1D;
        break;
    case DCT_FLIPADST:
        *type1 = DCT_1D;
        *type2 = FLIPADST_1D;
        break;
    case FLIPADST_FLIPADST:
        *type1 = FLIPADST_1D;
        *type2 = FLIPADST_1D;
        break;
    case ADST_FLIPADST:
        *type1 = ADST_1D;
        *type2 = FLIPADST_1D;
        break;
    case FLIPADST_ADST:
        *type1 = FLIPADST_1D;
        *type2 = ADST_1D;
        break;
    case IDTX:
        *type1 = IDTX_1D;
        *type2 = IDTX_1D;
        break;
    case H_DCT:
        *type1 = IDTX_1D;
        *type2 = DCT_1D;
        break;
    case V_DCT:
        *type1 = DCT_1D;
        *type2 = IDTX_1D;
        break;
    case H_ADST:
        *type1 = IDTX_1D;
        *type2 = ADST_1D;
        break;
    case V_ADST:
        *type1 = ADST_1D;
        *type2 = IDTX_1D;
        break;
    case H_FLIPADST:
        *type1 = IDTX_1D;
        *type2 = FLIPADST_1D;
        break;
    case V_FLIPADST:
        *type1 = FLIPADST_1D;
        *type2 = IDTX_1D;
        break;
    default:
        *type1 = DCT_1D;
        *type2 = DCT_1D;
        assert(0);
        break;
    }
}

INLINE void data_amplify(double *data, int size, int shift_bit) {
    double amplify_factor = static_cast<double>(
        shift_bit >= 0 ? (((uint32_t)1) << shift_bit)
                       : (1.0 / (((uint32_t)1) << (-shift_bit))));
    for (int i = 0; i < size; ++i)
        data[i] *= amplify_factor;
}

// forward transform 2d reference
INLINE void reference_txfm_2d(const double *in, double *out, TxType tx_type,
                              TxSize tx_size) {
    // Get transform type and size of each dimension.
    TX_TYPE_1D type1;
    TX_TYPE_1D type2;
    get_txfm2d_types(tx_type, &type1, &type2);
    const int tx_width = tx_size_wide[tx_size];
    const int tx_height = tx_size_high[tx_size];
    double *tmp_input = new double[tx_width * tx_height];
    double *tmp_output = new double[tx_width * tx_height];

    // second forward transform with type2
    for (int r = 0; r < tx_height; ++r) {
        reference_txfm_1d(
            type2, in + r * tx_width, out + r * tx_width, tx_width);
    }
    // matrix transposition
    for (int r = 0; r < tx_height; ++r) {
        for (int c = 0; c < tx_width; ++c) {
            tmp_input[c * tx_height + r] = out[r * tx_width + c];
        }
    }
    // first forward transform with type1
    for (int c = 0; c < tx_width; ++c) {
        reference_txfm_1d(type1,
                          tmp_input + c * tx_height,
                          tmp_output + c * tx_height,
                          tx_height);
    }
    // matrix transposition
    for (int r = 0; r < tx_height; ++r) {
        for (int c = 0; c < tx_width; ++c) {
            out[c * tx_height + r] = tmp_output[r * tx_width + c];
        }
    }
    if (tmp_input)
        delete[] tmp_input;
    if (tmp_output)
        delete[] tmp_output;
}

}  // namespace svt_av1_test_reference

#endif  // _TEST_REFERENCE_H_
