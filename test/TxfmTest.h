/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#ifndef _TXFM_TEST_H_
#define _TXFM_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

static const int8_t *fwd_txfm_range_mult2_list[TXFM_TYPES] = {
    fdct4_range_mult2,
    fdct8_range_mult2,
    fdct16_range_mult2,
    fdct32_range_mult2,
    fdct64_range_mult2,
    fadst4_range_mult2,
    fadst8_range_mult2,
    fadst16_range_mult2,
    fadst32_range_mult2,
    fidtx4_range_mult2,
    fidtx8_range_mult2,
    fidtx16_range_mult2,
    fidtx32_range_mult2,
    fidtx64_range_mult2};

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

typedef void (*TxfmFunc)(const int32_t *input, int32_t *output, int8_t cos_bit,
                         const int8_t *stage_range);

static INLINE TxfmFunc fwd_txfm_type_to_func(TXFM_TYPE txfm_type) {
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

static INLINE TxfmFunc inv_txfm_type_to_func(TXFM_TYPE txfm_type) {
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

#ifdef __cplusplus
}
#endif

#endif  // _TXFM_TEST_H_
