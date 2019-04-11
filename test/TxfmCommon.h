/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file TxfmCommon.h
 *
 * @brief declaration of fwd/inv txfm functions, including:
 * - 1d fwd/inv txfm function
 * - map type and size to function;
 *
 * @author Cidana-Edmond, Cidana-Wenyao
 *
 ******************************************************************************/
#ifndef _TXFM_TEST_H_
#define _TXFM_TEST_H_

#include "aom_dsp_rtcd.h"

#ifdef __cplusplus
extern "C" {
#endif

static const int8_t test_txfm_range[12] = {
    20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};

// export forward transform functions
void av1_fdct4_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                   const int8_t *stage_range);
void av1_fdct8_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                   const int8_t *stage_range);
void av1_fdct16_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_fdct32_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_fdct64_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_fadst4_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_fadst8_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_fadst16_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                     const int8_t *stage_range);
void av1_fadst32_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                     const int8_t *stage_range);
void av1_fidentity4_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                      const int8_t *stage_range);
void av1_fidentity8_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                      const int8_t *stage_range);
void av1_fidentity16_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);
void av1_fidentity32_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);
void av1_fidentity64_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);

// export inverse transform functions
void av1_idct4_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                   const int8_t *stage_range);
void av1_idct8_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                   const int8_t *stage_range);
void av1_idct16_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_idct32_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_idct64_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_iadst4_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_iadst8_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                    const int8_t *stage_range);
void av1_iadst16_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                     const int8_t *stage_range);
void av1_iadst32_new(const int32_t *input, int32_t *output, int8_t cos_bit,
                     const int8_t *stage_range);
void av1_iidentity4_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                      const int8_t *stage_range);
void av1_iidentity8_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                      const int8_t *stage_range);
void av1_iidentity16_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);
void av1_iidentity32_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);
void av1_iidentity64_c(const int32_t *input, int32_t *output, int8_t cos_bit,
                       const int8_t *stage_range);

void Av1TransformConfig(TxType tx_type, TxSize tx_size, Txfm2DFlipCfg *cfg);

typedef void (*Txfm1dFunc)(const int32_t *input, int32_t *output,
                           int8_t cos_bit, const int8_t *stage_range);

typedef void (*TxfmFwd2dFunc)(int16_t *input, int32_t *output,
                              uint32_t input_stride, TxType transform_type,
                              uint8_t bit_depth);

static INLINE Txfm1dFunc fwd_txfm_type_to_func(TxfmType txfm_type) {
    switch (txfm_type) {
    case TXFM_TYPE_DCT4: return av1_fdct4_new;
    case TXFM_TYPE_DCT8: return av1_fdct8_new;
    case TXFM_TYPE_DCT16: return av1_fdct16_new;
    case TXFM_TYPE_DCT32: return av1_fdct32_new;
    case TXFM_TYPE_DCT64: return av1_fdct64_new;
    case TXFM_TYPE_ADST4: return av1_fadst4_new;
    case TXFM_TYPE_ADST8: return av1_fadst8_new;
    case TXFM_TYPE_ADST16: return av1_fadst16_new;
    case TXFM_TYPE_ADST32: return av1_fadst32_new;
    case TXFM_TYPE_IDENTITY4: return av1_fidentity4_c;
    case TXFM_TYPE_IDENTITY8: return av1_fidentity8_c;
    case TXFM_TYPE_IDENTITY16: return av1_fidentity16_c;
    case TXFM_TYPE_IDENTITY32: return av1_fidentity32_c;
    case TXFM_TYPE_IDENTITY64: return av1_fidentity64_c;
    default: assert(0); return NULL;
    }
}

static INLINE Txfm1dFunc inv_txfm_type_to_func(TxfmType txfm_type) {
    switch (txfm_type) {
    case TXFM_TYPE_DCT4: return av1_idct4_new;
    case TXFM_TYPE_DCT8: return av1_idct8_new;
    case TXFM_TYPE_DCT16: return av1_idct16_new;
    case TXFM_TYPE_DCT32: return av1_idct32_new;
    case TXFM_TYPE_DCT64: return av1_idct64_new;
    case TXFM_TYPE_ADST4: return av1_iadst4_new;
    case TXFM_TYPE_ADST8: return av1_iadst8_new;
    case TXFM_TYPE_ADST16: return av1_iadst16_new;
    case TXFM_TYPE_ADST32: return av1_iadst32_new;
    case TXFM_TYPE_IDENTITY4: return av1_iidentity4_c;
    case TXFM_TYPE_IDENTITY8: return av1_iidentity8_c;
    case TXFM_TYPE_IDENTITY16: return av1_iidentity16_c;
    case TXFM_TYPE_IDENTITY32: return av1_iidentity32_c;
    case TXFM_TYPE_IDENTITY64: return av1_iidentity64_c;
    default: assert(0); return NULL;
    }
}

static INLINE TxfmFwd2dFunc fwd_txfm_2d_size_to_func(TxSize txfm_size) {
    switch (txfm_size) {
    case TX_4X4:
        return Av1TransformTwoD_4x4_c;  // 4x4 transform
    case TX_8X8:
        return Av1TransformTwoD_8x8_c;  // 8x8 transform
    case TX_16X16:
        return Av1TransformTwoD_16x16_c;  // 16x16 transform
    case TX_32X32:
        return Av1TransformTwoD_32x32_c;  // 32x32 transform
    case TX_64X64:
        return Av1TransformTwoD_64x64_c;  // 64x64 transform
    case TX_4X8:
        return av1_fwd_txfm2d_4x8_c;  // 4x8 transform
    case TX_8X4:
        return av1_fwd_txfm2d_8x4_c;  // 8x4 transform
    case TX_8X16:
        return av1_fwd_txfm2d_8x16_c;  // 8x16 transform
    case TX_16X8:
        return av1_fwd_txfm2d_16x8_c;  // 16x8 transform
    case TX_16X32:
        return av1_fwd_txfm2d_16x32_c;  // 16x32 transform
    case TX_32X16:
        return av1_fwd_txfm2d_32x16_c;  // 32x16 transform
    case TX_32X64:
        return av1_fwd_txfm2d_32x64_c;  // 32x64 transform
    case TX_64X32:
        return av1_fwd_txfm2d_64x32_c;  // 64x32 transform
    case TX_4X16:
        return av1_fwd_txfm2d_4x16_c;  // 4x16 transform
    case TX_16X4:
        return av1_fwd_txfm2d_16x4_c;  // 16x4 transform
    case TX_8X32:
        return av1_fwd_txfm2d_8x32_c;  // 8x32 transform
    case TX_32X8:
        return av1_fwd_txfm2d_32x8_c;  // 32x8 transform
    case TX_16X64:
        return av1_fwd_txfm2d_16x64_c;  // 16x64 transform
    case TX_64X16:
        return av1_fwd_txfm2d_64x16_c;  // 64x16 transform
    default: assert(0); return NULL;
    }
}

static INLINE std::string fwd_txfm_2d_size_to_name(TxSize txfm_size) {
    switch (txfm_size) {
    case TX_4X4:
        return "Av1TransformTwoD_4x4_c";  // 4x4 transform
    case TX_8X8:
        return "Av1TransformTwoD_8x8_c";  // 8x8 transform
    case TX_16X16:
        return "Av1TransformTwoD_16x16_c";  // 16x16 transform
    case TX_32X32:
        return "Av1TransformTwoD_32x32_c";  // 32x32 transform
    case TX_64X64:
        return "Av1TransformTwoD_64x64_c";  // 64x64 transform
    case TX_4X8:
        return "av1_fwd_txfm2d_4x8_c";  // 4x8 transform
    case TX_8X4:
        return "av1_fwd_txfm2d_8x4_c";  // 8x4 transform
    case TX_8X16:
        return "av1_fwd_txfm2d_8x16_c";  // 8x16 transform
    case TX_16X8:
        return "av1_fwd_txfm2d_16x8_c";  // 16x8 transform
    case TX_16X32:
        return "av1_fwd_txfm2d_16x32_c";  // 16x32 transform
    case TX_32X16:
        return "av1_fwd_txfm2d_32x16_c";  // 32x16 transform
    case TX_32X64:
        return "av1_fwd_txfm2d_32x64_c";  // 32x64 transform
    case TX_64X32:
        return "av1_fwd_txfm2d_64x32_c";  // 64x32 transform
    case TX_4X16:
        return "av1_fwd_txfm2d_4x16_c";  // 4x16 transform
    case TX_16X4:
        return "av1_fwd_txfm2d_16x4_c";  // 16x4 transform
    case TX_8X32:
        return "av1_fwd_txfm2d_8x32_c";  // 8x32 transform
    case TX_32X8:
        return "av1_fwd_txfm2d_32x8_c";  // 32x8 transform
    case TX_16X64:
        return "av1_fwd_txfm2d_16x64_c";  // 16x64 transform
    case TX_64X16:
        return "av1_fwd_txfm2d_64x16_c";  // 64x16 transform
    default: assert(0); return "";
    }
}

static INLINE std::string fwd_txfm_2d_type_to_name(TxType txfm_type) {
    switch (txfm_type) {
    case DCT_DCT: return "DCT_DCT";
    case ADST_DCT: return "ADST_DCT";
    case DCT_ADST: return "DCT_ADST";
    case ADST_ADST: return "ADST_ADST";
    case FLIPADST_DCT: return "FLIPADST_DCT";
    case DCT_FLIPADST: return "DCT_FLIPADST";
    case FLIPADST_FLIPADST: return "FLIPADST_FLIPADST";
    case ADST_FLIPADST: return "ADST_FLIPADST";
    case FLIPADST_ADST: return "FLIPADST_ADST";
    case IDTX: return "IDTX";
    case V_DCT: return "V_DCT";
    case H_DCT: return "H_DCT";
    case V_ADST: return "V_ADST";
    case H_ADST: return "H_ADST";
    case V_FLIPADST: return "V_FLIPADST";
    case H_FLIPADST: return "H_FLIPADST";
    default: assert(0); return "";
    }
}

static INLINE bool valid_txsize_txtype(TxSize txfm_size, TxType txfm_type) {
    const TxSize txfm_size_sqr_up = txsize_sqr_up_map[txfm_size];
    TxSetType txfm_set_type;
    if (txfm_size_sqr_up > TX_32X32) {
        txfm_set_type = EXT_TX_SET_DCTONLY;
    } else if (txfm_size_sqr_up == TX_32X32) {
        txfm_set_type = EXT_TX_SET_DCT_IDTX;
    } else {
        txfm_set_type = EXT_TX_SET_ALL16;
    }
    return av1_ext_tx_used[txfm_set_type][txfm_type] != 0;
}

static INLINE int get_txfm1d_size(TxfmType txfm_type) {
    switch (txfm_type) {
    case TXFM_TYPE_DCT4:
    case TXFM_TYPE_ADST4:
    case TXFM_TYPE_IDENTITY4: return 4;
    case TXFM_TYPE_DCT8:
    case TXFM_TYPE_ADST8:
    case TXFM_TYPE_IDENTITY8: return 8;
    case TXFM_TYPE_DCT16:
    case TXFM_TYPE_ADST16:
    case TXFM_TYPE_IDENTITY16: return 16;
    case TXFM_TYPE_DCT32:
    case TXFM_TYPE_ADST32:
    case TXFM_TYPE_IDENTITY32: return 32;
    case TXFM_TYPE_DCT64:
    case TXFM_TYPE_IDENTITY64: return 64;
    default: assert(0); return 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif  // _TXFM_TEST_H_
