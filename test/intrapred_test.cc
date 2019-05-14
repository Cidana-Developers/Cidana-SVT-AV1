/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <string>

#include "gtest/gtest.h"

#include "aom_dsp_rtcd.h"
#include "EbDefinitions.h"
#include "random.h"
namespace {
using svt_av1_test_tool::SVTRandom;

const int count_test_block = 100000;

typedef void (*HighbdIntraPred)(uint16_t *dst, ptrdiff_t stride,
                                const uint16_t *above, const uint16_t *left,
                                int bd);
typedef void (*IntraPred)(uint8_t *dst, ptrdiff_t stride, const uint8_t *above,
                          const uint8_t *left);

template <typename FuncType>
struct IntraPredFunc {
    IntraPredFunc(FuncType pred = NULL, FuncType ref = NULL,
                  int block_width_value = 0, int block_height_value = 0,
                  int bit_depth_value = 0)
        : pred_fn(pred),
          ref_fn(ref),
          block_width(block_width_value),
          block_height(block_height_value),
          bit_depth(bit_depth_value) {
    }

    FuncType pred_fn;
    FuncType ref_fn;
    int block_width;
    int block_height;
    int bit_depth;
};

template <typename FuncType, typename Pixel>
class AV1IntraPredTest
    : public ::testing::TestWithParam<IntraPredFunc<FuncType>> {
  public:
    void RunTest(Pixel *left_col, Pixel *above_data, Pixel *dst,
                 Pixel *ref_dst) {
        SVTRandom rnd(0, (1 << params_.bit_depth) - 1);
        const int block_width = params_.block_width;
        const int block_height = params_.block_height;
        above_row_ = above_data + 16;
        left_col_ = left_col;
        dst_ = dst;
        ref_dst_ = ref_dst;
        int error_count = 0;
        for (int i = 0; i < count_test_block; ++i) {
            // Fill edges with random data, try first with saturated values.
            for (int x = -1; x <= block_width * 2; x++) {
                if (i == 0) {
                    above_row_[x] = mask_;
                } else {
                    above_row_[x] = rnd.random();
                }
            }
            for (int y = 0; y < block_height; y++) {
                if (i == 0) {
                    left_col_[y] = mask_;
                } else {
                    left_col_[y] = rnd.random();
                }
            }
            Predict();
            CheckPrediction(i, &error_count);
        }
        ASSERT_EQ(0, error_count);
    }

  protected:
    virtual void SetUp() {
        params_ = this->GetParam();
        stride_ = params_.block_width * 3;
        mask_ = (1 << params_.bit_depth) - 1;
    }

    virtual void Predict() = 0;

    void CheckPrediction(int test_case_number, int *error_count) const {
        // For each pixel ensure that the calculated value is the same as
        // reference.
        const int block_width = params_.block_width;
        const int block_height = params_.block_height;
        for (int y = 0; y < block_height; y++) {
            for (int x = 0; x < block_width; x++) {
                *error_count +=
                    ref_dst_[x + y * stride_] != dst_[x + y * stride_];
                if (*error_count == 1) {
                    ASSERT_EQ(ref_dst_[x + y * stride_], dst_[x + y * stride_])
                        << " Failed on Test Case Number " << test_case_number
                        << " location: x = " << x << " y = " << y;
                }
            }
        }
    }

    Pixel *above_row_;
    Pixel *left_col_;
    Pixel *dst_;
    Pixel *ref_dst_;
    ptrdiff_t stride_;
    int mask_;

    IntraPredFunc<FuncType> params_;
};

class HighbdIntraPredTest : public AV1IntraPredTest<HighbdIntraPred, uint16_t> {
  protected:
    void Predict() {
        const int bit_depth = params_.bit_depth;
        params_.ref_fn(ref_dst_, stride_, above_row_, left_col_, bit_depth);
        params_.pred_fn(dst_, stride_, above_row_, left_col_, bit_depth);
    }
};

class LowbdIntraPredTest : public AV1IntraPredTest<IntraPred, uint8_t> {
  protected:
    void Predict() {
        params_.ref_fn(ref_dst_, stride_, above_row_, left_col_);
        params_.pred_fn(dst_, stride_, above_row_, left_col_);
    }
};

// Suppress an unitialized warning. Once there are implementations to test then
// this can be restored.
TEST_P(HighbdIntraPredTest, Bitexact) {
    // max block size is 64
    DECLARE_ALIGNED(16, uint16_t, left_col[2 * 64]);
    DECLARE_ALIGNED(16, uint16_t, above_data[2 * 64 + 64]);
    DECLARE_ALIGNED(16, uint16_t, dst[3 * 64 * 64]);
    DECLARE_ALIGNED(16, uint16_t, ref_dst[3 * 64 * 64]);
    av1_zero(left_col);
    av1_zero(above_data);
    RunTest(left_col, above_data, dst, ref_dst);
}

// Same issue as above but for arm.
TEST_P(LowbdIntraPredTest, Bitexact) {
    // max block size is 32
    DECLARE_ALIGNED(16, uint8_t, left_col[2 * 32]);
    DECLARE_ALIGNED(16, uint8_t, above_data[2 * 32 + 32]);
    DECLARE_ALIGNED(16, uint8_t, dst[3 * 32 * 32]);
    DECLARE_ALIGNED(16, uint8_t, ref_dst[3 * 32 * 32]);
    av1_zero(left_col);
    av1_zero(above_data);
    RunTest(left_col, above_data, dst, ref_dst);
}

// -----------------------------------------------------------------------------
// High Bit Depth Tests
#define highbd_entry(type, width, height, opt, bd)                 \
    IntraPredFunc<HighbdIntraPred>(                                \
        &aom_highbd_##type##_predictor_##width##x##height##_##opt, \
        &aom_highbd_##type##_predictor_##width##x##height##_c,     \
        width,                                                     \
        height,                                                    \
        bd)

const IntraPredFunc<HighbdIntraPred> HighbdIntraPredTestVectorAsm[] = {
    highbd_entry(dc_128, 16, 16, avx2, 10),
    highbd_entry(dc_128, 16, 32, avx2, 10),
    highbd_entry(dc_128, 16, 4, avx2, 10),
    highbd_entry(dc_128, 16, 64, avx2, 10),
    highbd_entry(dc_128, 16, 8, avx2, 10),
    highbd_entry(dc_128, 32, 16, avx2, 10),
    highbd_entry(dc_128, 32, 32, avx2, 10),
    highbd_entry(dc_128, 32, 64, avx2, 10),
    highbd_entry(dc_128, 32, 8, avx2, 10),
    highbd_entry(dc_128, 4, 16, sse2, 10),
    highbd_entry(dc_128, 4, 4, sse2, 10),
    highbd_entry(dc_128, 4, 8, sse2, 10),
    highbd_entry(dc_128, 64, 16, avx2, 10),
    highbd_entry(dc_128, 64, 32, avx2, 10),
    highbd_entry(dc_128, 64, 64, avx2, 10),
    highbd_entry(dc_128, 8, 16, sse2, 10),
    highbd_entry(dc_128, 8, 32, sse2, 10),
    highbd_entry(dc_128, 8, 4, sse2, 10),
    highbd_entry(dc_128, 8, 8, sse2, 10),
    highbd_entry(dc_left, 16, 16, avx2, 10),
    highbd_entry(dc_left, 16, 32, avx2, 10),
    highbd_entry(dc_left, 16, 4, avx2, 10),
    highbd_entry(dc_left, 16, 64, avx2, 10),
    highbd_entry(dc_left, 16, 8, avx2, 10),
    highbd_entry(dc_left, 32, 16, avx2, 10),
    highbd_entry(dc_left, 32, 32, avx2, 10),
    highbd_entry(dc_left, 32, 64, avx2, 10),
    highbd_entry(dc_left, 32, 8, avx2, 10),
    highbd_entry(dc_left, 4, 16, sse2, 10),
    highbd_entry(dc_left, 4, 4, sse2, 10),
    highbd_entry(dc_left, 4, 8, sse2, 10),
    highbd_entry(dc_left, 64, 16, avx2, 10),
    highbd_entry(dc_left, 64, 32, avx2, 10),
    highbd_entry(dc_left, 64, 64, avx2, 10),
    highbd_entry(dc_left, 8, 16, sse2, 10),
    highbd_entry(dc_left, 8, 32, sse2, 10),
    highbd_entry(dc_left, 8, 4, sse2, 10),
    highbd_entry(dc_left, 8, 8, sse2, 10),
    highbd_entry(dc, 16, 16, avx2, 10),
    highbd_entry(dc, 16, 32, avx2, 10),
    highbd_entry(dc, 16, 4, avx2, 10),
    highbd_entry(dc, 16, 64, avx2, 10),
    highbd_entry(dc, 16, 8, avx2, 10),
    highbd_entry(dc, 32, 16, avx2, 10),
    highbd_entry(dc, 32, 32, avx2, 10),
    highbd_entry(dc, 32, 64, avx2, 10),
    highbd_entry(dc, 32, 8, avx2, 10),
    highbd_entry(dc, 4, 16, sse2, 10),
    highbd_entry(dc, 4, 4, sse2, 10),
    highbd_entry(dc, 4, 8, sse2, 10),
    highbd_entry(dc, 64, 16, avx2, 10),
    highbd_entry(dc, 64, 32, avx2, 10),
    highbd_entry(dc, 64, 64, avx2, 10),
    highbd_entry(dc, 8, 16, sse2, 10),
    highbd_entry(dc, 8, 32, sse2, 10),
    highbd_entry(dc, 8, 4, sse2, 10),
    highbd_entry(dc, 8, 8, sse2, 10),
    highbd_entry(dc_top, 16, 16, avx2, 10),
    highbd_entry(dc_top, 16, 32, avx2, 10),
    highbd_entry(dc_top, 16, 4, avx2, 10),
    highbd_entry(dc_top, 16, 64, avx2, 10),
    highbd_entry(dc_top, 16, 8, avx2, 10),
    highbd_entry(dc_top, 32, 16, avx2, 10),
    highbd_entry(dc_top, 32, 32, avx2, 10),
    highbd_entry(dc_top, 32, 64, avx2, 10),
    highbd_entry(dc_top, 32, 8, avx2, 10),
    highbd_entry(dc_top, 4, 16, sse2, 10),
    highbd_entry(dc_top, 4, 4, sse2, 10),
    highbd_entry(dc_top, 4, 8, sse2, 10),
    highbd_entry(dc_top, 64, 16, avx2, 10),
    highbd_entry(dc_top, 64, 32, avx2, 10),
    highbd_entry(dc_top, 64, 64, avx2, 10),
    highbd_entry(dc_top, 8, 16, sse2, 10),
    highbd_entry(dc_top, 8, 4, sse2, 10),
    highbd_entry(dc_top, 8, 8, sse2, 10),
    highbd_entry(h, 16, 16, sse2, 10),
    highbd_entry(h, 16, 32, sse2, 10),
    highbd_entry(h, 16, 4, avx2, 10),
    highbd_entry(h, 16, 64, avx2, 10),
    highbd_entry(h, 16, 8, sse2, 10),
    highbd_entry(h, 32, 16, sse2, 10),
    highbd_entry(h, 32, 32, sse2, 10),
    highbd_entry(h, 32, 64, avx2, 10),
    highbd_entry(h, 32, 8, avx2, 10),
    highbd_entry(h, 4, 16, sse2, 10),
    highbd_entry(h, 4, 4, sse2, 10),
    highbd_entry(h, 4, 8, sse2, 10),
    highbd_entry(h, 64, 16, avx2, 10),
    highbd_entry(h, 64, 32, avx2, 10),
    highbd_entry(h, 64, 64, avx2, 10),
    highbd_entry(h, 8, 16, sse2, 10),
    highbd_entry(h, 8, 32, sse2, 10),
    highbd_entry(h, 8, 4, sse2, 10),
    highbd_entry(h, 8, 8, sse2, 10),
    highbd_entry(smooth_h, 16, 16, avx2, 10),
    highbd_entry(smooth_h, 16, 32, avx2, 10),
    highbd_entry(smooth_h, 16, 4, avx2, 10),
    highbd_entry(smooth_h, 16, 64, avx2, 10),
    highbd_entry(smooth_h, 16, 8, avx2, 10),
    highbd_entry(smooth_h, 32, 16, avx2, 10),
    highbd_entry(smooth_h, 32, 32, avx2, 10),
    highbd_entry(smooth_h, 32, 64, avx2, 10),
    highbd_entry(smooth_h, 32, 8, avx2, 10),
    highbd_entry(smooth_h, 4, 16, ssse3, 10),
    highbd_entry(smooth_h, 4, 4, ssse3, 10),
    highbd_entry(smooth_h, 4, 8, ssse3, 10),
    highbd_entry(smooth_h, 64, 16, avx2, 10),
    highbd_entry(smooth_h, 64, 32, avx2, 10),
    highbd_entry(smooth_h, 64, 64, avx2, 10),
    highbd_entry(smooth_h, 8, 16, avx2, 10),
    highbd_entry(smooth_h, 8, 32, avx2, 10),
    highbd_entry(smooth_h, 8, 4, avx2, 10),
    highbd_entry(smooth_h, 8, 8, avx2, 10),
    highbd_entry(smooth, 16, 16, avx2, 10),
    highbd_entry(smooth, 16, 32, avx2, 10),
    highbd_entry(smooth, 16, 4, avx2, 10),
    highbd_entry(smooth, 16, 64, avx2, 10),
    highbd_entry(smooth, 16, 8, avx2, 10),
    highbd_entry(smooth, 32, 16, avx2, 10),
    highbd_entry(smooth, 32, 32, avx2, 10),
    highbd_entry(smooth, 32, 64, avx2, 10),
    highbd_entry(smooth, 32, 8, avx2, 10),
    highbd_entry(smooth, 4, 16, ssse3, 10),
    highbd_entry(smooth, 4, 4, ssse3, 10),
    highbd_entry(smooth, 4, 8, ssse3, 10),
    highbd_entry(smooth, 64, 16, avx2, 10),
    highbd_entry(smooth, 64, 32, avx2, 10),
    highbd_entry(smooth, 64, 64, avx2, 10),
    highbd_entry(smooth, 8, 16, avx2, 10),
    highbd_entry(smooth, 8, 32, avx2, 10),
    highbd_entry(smooth, 8, 4, avx2, 10),
    highbd_entry(smooth, 8, 8, avx2, 10),
    highbd_entry(smooth_v, 16, 16, avx2, 10),
    highbd_entry(smooth_v, 16, 32, avx2, 10),
    highbd_entry(smooth_v, 16, 4, avx2, 10),
    highbd_entry(smooth_v, 16, 64, avx2, 10),
    highbd_entry(smooth_v, 16, 8, avx2, 10),
    highbd_entry(smooth_v, 32, 16, avx2, 10),
    highbd_entry(smooth_v, 32, 32, avx2, 10),
    highbd_entry(smooth_v, 32, 64, avx2, 10),
    highbd_entry(smooth_v, 32, 8, avx2, 10),
    highbd_entry(smooth_v, 4, 16, ssse3, 10),
    highbd_entry(smooth_v, 4, 4, ssse3, 10),
    highbd_entry(smooth_v, 4, 8, ssse3, 10),
    highbd_entry(smooth_v, 64, 16, avx2, 10),
    highbd_entry(smooth_v, 64, 32, avx2, 10),
    highbd_entry(smooth_v, 64, 64, avx2, 10),
    highbd_entry(smooth_v, 8, 16, avx2, 10),
    highbd_entry(smooth_v, 8, 32, avx2, 10),
    highbd_entry(smooth_v, 8, 4, avx2, 10),
    highbd_entry(smooth_v, 8, 8, avx2, 10),
    highbd_entry(v, 16, 16, avx2, 10),
    highbd_entry(v, 16, 32, avx2, 10),
    highbd_entry(v, 16, 4, avx2, 10),
    highbd_entry(v, 16, 64, avx2, 10),
    highbd_entry(v, 16, 8, avx2, 10),
    highbd_entry(v, 32, 16, avx2, 10),
    highbd_entry(v, 32, 32, avx2, 10),
    highbd_entry(v, 32, 64, avx2, 10),
    highbd_entry(v, 32, 8, avx2, 10),
    highbd_entry(v, 4, 16, sse2, 10),
    highbd_entry(v, 4, 4, sse2, 10),
    highbd_entry(v, 4, 8, sse2, 10),
    highbd_entry(v, 64, 16, avx2, 10),
    highbd_entry(v, 64, 32, avx2, 10),
    highbd_entry(v, 64, 64, avx2, 10),
    highbd_entry(v, 8, 16, sse2, 10),
    highbd_entry(v, 8, 32, sse2, 10),
    highbd_entry(v, 8, 4, sse2, 10),
    highbd_entry(v, 8, 8, sse2, 10),
};

INSTANTIATE_TEST_CASE_P(highbd_intra, HighbdIntraPredTest,
                        ::testing::ValuesIn(HighbdIntraPredTestVectorAsm));

// ---------------------------------------------------------------------------
// Low Bit Depth Tests

#define lowbd_entry(type, width, height, opt)               \
    IntraPredFunc<IntraPred>(                               \
        &aom_##type##_predictor_##width##x##height##_##opt, \
        &aom_##type##_predictor_##width##x##height##_c,     \
        width,                                              \
        height,                                             \
        8)

const IntraPredFunc<IntraPred> LowbdIntraPredTestVectorAsm[] = {
    lowbd_entry(dc, 4, 4, sse2),          lowbd_entry(dc, 8, 8, sse2),
    lowbd_entry(dc, 16, 16, sse2),        lowbd_entry(dc, 32, 32, avx2),
    lowbd_entry(dc, 64, 64, avx2),        lowbd_entry(dc, 16, 32, sse2),
    lowbd_entry(dc, 16, 4, sse2),         lowbd_entry(dc, 16, 64, sse2),
    lowbd_entry(dc, 16, 8, sse2),         lowbd_entry(dc, 32, 16, avx2),
    lowbd_entry(dc, 32, 64, avx2),        lowbd_entry(dc, 32, 8, sse2),
    lowbd_entry(dc, 4, 16, sse2),         lowbd_entry(dc, 4, 8, sse2),
    lowbd_entry(dc, 64, 16, avx2),        lowbd_entry(dc, 64, 32, avx2),
    lowbd_entry(dc, 8, 16, sse2),         lowbd_entry(dc, 8, 32, sse2),
    lowbd_entry(dc, 8, 4, sse2),          lowbd_entry(dc_left, 4, 4, sse2),
    lowbd_entry(dc_left, 8, 8, sse2),     lowbd_entry(dc_left, 16, 16, sse2),
    lowbd_entry(dc_left, 32, 32, avx2),   lowbd_entry(dc_left, 64, 64, avx2),
    lowbd_entry(dc_left, 16, 32, sse2),   lowbd_entry(dc_left, 16, 4, sse2),
    lowbd_entry(dc_left, 16, 64, sse2),   lowbd_entry(dc_left, 16, 8, sse2),
    lowbd_entry(dc_left, 32, 16, avx2),   lowbd_entry(dc_left, 32, 64, avx2),
    lowbd_entry(dc_left, 32, 8, sse2),    lowbd_entry(dc_left, 4, 16, sse2),
    lowbd_entry(dc_left, 4, 8, sse2),     lowbd_entry(dc_left, 64, 16, avx2),
    lowbd_entry(dc_left, 64, 32, avx2),   lowbd_entry(dc_left, 8, 16, sse2),
    lowbd_entry(dc_left, 8, 32, sse2),    lowbd_entry(dc_left, 8, 4, sse2),
    lowbd_entry(dc_top, 4, 4, sse2),      lowbd_entry(dc_top, 8, 8, sse2),
    lowbd_entry(dc_top, 16, 16, sse2),    lowbd_entry(dc_top, 32, 32, avx2),
    lowbd_entry(dc_top, 64, 64, avx2),    lowbd_entry(dc_top, 16, 32, sse2),
    lowbd_entry(dc_top, 16, 4, sse2),     lowbd_entry(dc_top, 16, 64, sse2),
    lowbd_entry(dc_top, 16, 8, sse2),     lowbd_entry(dc_top, 32, 16, avx2),
    lowbd_entry(dc_top, 32, 64, avx2),    lowbd_entry(dc_top, 32, 8, sse2),
    lowbd_entry(dc_top, 4, 16, sse2),     lowbd_entry(dc_top, 4, 8, sse2),
    lowbd_entry(dc_top, 64, 16, avx2),    lowbd_entry(dc_top, 64, 32, avx2),
    lowbd_entry(dc_top, 8, 16, sse2),     lowbd_entry(dc_top, 8, 32, sse2),
    lowbd_entry(dc_top, 8, 4, sse2),      lowbd_entry(dc_128, 4, 4, sse2),
    lowbd_entry(dc_128, 8, 8, sse2),      lowbd_entry(dc_128, 16, 16, sse2),
    lowbd_entry(dc_128, 32, 32, avx2),    lowbd_entry(dc_128, 64, 64, avx2),
    lowbd_entry(dc_128, 16, 32, sse2),    lowbd_entry(dc_128, 16, 4, sse2),
    lowbd_entry(dc_128, 16, 64, sse2),    lowbd_entry(dc_128, 16, 8, sse2),
    lowbd_entry(dc_128, 32, 16, avx2),    lowbd_entry(dc_128, 32, 64, avx2),
    lowbd_entry(dc_128, 32, 8, sse2),     lowbd_entry(dc_128, 4, 16, sse2),
    lowbd_entry(dc_128, 4, 8, sse2),      lowbd_entry(dc_128, 64, 16, avx2),
    lowbd_entry(dc_128, 64, 32, avx2),    lowbd_entry(dc_128, 8, 16, sse2),
    lowbd_entry(dc_128, 8, 32, sse2),     lowbd_entry(dc_128, 8, 4, sse2),
    lowbd_entry(smooth_h, 64, 64, ssse3), lowbd_entry(smooth_h, 32, 32, ssse3),
    lowbd_entry(smooth_h, 16, 16, ssse3), lowbd_entry(smooth_h, 8, 8, ssse3),
    lowbd_entry(smooth_h, 4, 4, ssse3),   lowbd_entry(smooth_h, 16, 32, ssse3),
    lowbd_entry(smooth_h, 16, 4, ssse3),  lowbd_entry(smooth_h, 16, 64, ssse3),
    lowbd_entry(smooth_h, 16, 8, ssse3),  lowbd_entry(smooth_h, 32, 16, ssse3),
    lowbd_entry(smooth_h, 32, 64, ssse3), lowbd_entry(smooth_h, 32, 8, ssse3),
    lowbd_entry(smooth_h, 4, 16, ssse3),  lowbd_entry(smooth_h, 4, 8, ssse3),
    lowbd_entry(smooth_h, 64, 16, ssse3), lowbd_entry(smooth_h, 64, 32, ssse3),
    lowbd_entry(smooth_h, 8, 16, ssse3),  lowbd_entry(smooth_h, 8, 32, ssse3),
    lowbd_entry(smooth_h, 8, 4, ssse3),   lowbd_entry(smooth_v, 64, 64, ssse3),
    lowbd_entry(smooth_v, 32, 32, ssse3), lowbd_entry(smooth_v, 16, 16, ssse3),
    lowbd_entry(smooth_v, 8, 8, ssse3),   lowbd_entry(smooth_v, 4, 4, ssse3),
    lowbd_entry(smooth_v, 16, 32, ssse3), lowbd_entry(smooth_v, 16, 4, ssse3),
    lowbd_entry(smooth_v, 16, 64, ssse3), lowbd_entry(smooth_v, 16, 8, ssse3),
    lowbd_entry(smooth_v, 32, 16, ssse3), lowbd_entry(smooth_v, 32, 64, ssse3),
    lowbd_entry(smooth_v, 32, 8, ssse3),  lowbd_entry(smooth_v, 4, 16, ssse3),
    lowbd_entry(smooth_v, 4, 8, ssse3),   lowbd_entry(smooth_v, 64, 16, ssse3),
    lowbd_entry(smooth_v, 64, 32, ssse3), lowbd_entry(smooth_v, 8, 16, ssse3),
    lowbd_entry(smooth_v, 8, 32, ssse3),  lowbd_entry(smooth_v, 8, 4, ssse3),
    lowbd_entry(smooth, 64, 64, ssse3),   lowbd_entry(smooth, 32, 32, ssse3),
    lowbd_entry(smooth, 16, 16, ssse3),   lowbd_entry(smooth, 8, 8, ssse3),
    lowbd_entry(smooth, 4, 4, ssse3),     lowbd_entry(smooth, 16, 32, ssse3),
    lowbd_entry(smooth, 16, 4, ssse3),    lowbd_entry(smooth, 16, 64, ssse3),
    lowbd_entry(smooth, 16, 8, ssse3),    lowbd_entry(smooth, 32, 16, ssse3),
    lowbd_entry(smooth, 32, 64, ssse3),   lowbd_entry(smooth, 32, 8, ssse3),
    lowbd_entry(smooth, 4, 16, ssse3),    lowbd_entry(smooth, 4, 8, ssse3),
    lowbd_entry(smooth, 64, 16, ssse3),   lowbd_entry(smooth, 64, 32, ssse3),
    lowbd_entry(smooth, 8, 16, ssse3),    lowbd_entry(smooth, 8, 32, ssse3),
    lowbd_entry(smooth, 8, 4, ssse3),     lowbd_entry(v, 4, 4, sse2),
    lowbd_entry(v, 8, 8, sse2),           lowbd_entry(v, 16, 16, sse2),
    lowbd_entry(v, 32, 32, avx2),         lowbd_entry(v, 64, 64, avx2),
    lowbd_entry(v, 16, 32, sse2),         lowbd_entry(v, 16, 4, sse2),
    lowbd_entry(v, 16, 64, sse2),         lowbd_entry(v, 16, 8, sse2),
    lowbd_entry(v, 32, 16, avx2),         lowbd_entry(v, 32, 64, avx2),
    lowbd_entry(v, 32, 8, sse2),          lowbd_entry(v, 4, 16, sse2),
    lowbd_entry(v, 4, 8, sse2),           lowbd_entry(v, 64, 16, avx2),
    lowbd_entry(v, 64, 32, avx2),         lowbd_entry(v, 8, 16, sse2),
    lowbd_entry(v, 8, 32, sse2),          lowbd_entry(v, 8, 4, sse2),
    lowbd_entry(h, 4, 4, sse2),           lowbd_entry(h, 8, 8, sse2),
    lowbd_entry(h, 16, 16, sse2),         lowbd_entry(h, 32, 32, avx2),
    lowbd_entry(h, 64, 64, sse2),         lowbd_entry(h, 16, 32, sse2),
    lowbd_entry(h, 16, 4, sse2),          lowbd_entry(h, 16, 64, sse2),
    lowbd_entry(h, 16, 8, sse2),          lowbd_entry(h, 32, 16, sse2),
    lowbd_entry(h, 32, 64, sse2),         lowbd_entry(h, 32, 8, sse2),
    lowbd_entry(h, 4, 16, sse2),          lowbd_entry(h, 4, 8, sse2),
    lowbd_entry(h, 64, 16, sse2),         lowbd_entry(h, 64, 32, sse2),
    lowbd_entry(h, 8, 16, sse2),          lowbd_entry(h, 8, 32, sse2),
    lowbd_entry(h, 8, 4, sse2),           lowbd_entry(paeth, 16, 16, ssse3),
    lowbd_entry(paeth, 16, 16, avx2),     lowbd_entry(paeth, 16, 32, ssse3),
    lowbd_entry(paeth, 16, 32, avx2),     lowbd_entry(paeth, 16, 4, ssse3),
    lowbd_entry(paeth, 16, 64, ssse3),    lowbd_entry(paeth, 16, 64, avx2),
    lowbd_entry(paeth, 16, 8, ssse3),     lowbd_entry(paeth, 16, 8, avx2),
    lowbd_entry(paeth, 32, 16, ssse3),    lowbd_entry(paeth, 32, 16, avx2),
    lowbd_entry(paeth, 32, 32, ssse3),    lowbd_entry(paeth, 32, 32, avx2),
    lowbd_entry(paeth, 32, 64, ssse3),    lowbd_entry(paeth, 32, 64, avx2),
    lowbd_entry(paeth, 32, 8, ssse3),     lowbd_entry(paeth, 4, 16, ssse3),
    lowbd_entry(paeth, 4, 4, ssse3),      lowbd_entry(paeth, 4, 8, ssse3),
    lowbd_entry(paeth, 64, 16, ssse3),    lowbd_entry(paeth, 64, 16, avx2),
    lowbd_entry(paeth, 64, 32, ssse3),    lowbd_entry(paeth, 64, 32, avx2),
    lowbd_entry(paeth, 64, 64, ssse3),    lowbd_entry(paeth, 64, 64, avx2),
    lowbd_entry(paeth, 8, 16, ssse3),     lowbd_entry(paeth, 8, 32, ssse3),
    lowbd_entry(paeth, 8, 4, ssse3),      lowbd_entry(paeth, 8, 8, ssse3),
};

INSTANTIATE_TEST_CASE_P(lowbd_intra, LowbdIntraPredTest,
                        ::testing::ValuesIn(LowbdIntraPredTestVectorAsm));
}  // namespace
