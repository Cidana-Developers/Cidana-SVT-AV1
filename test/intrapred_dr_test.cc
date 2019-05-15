/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */
#include "gtest/gtest.h"

#include "aom_dsp_rtcd.h"
#include "EbDefinitions.h"
#include "random.h"

namespace {
const uint16_t dr_intra_derivative[90] = {
    // More evenly spread out angles and limited to 10-bit
    // Values that are 0 will never be used
    //                    Approx angle
    0,    0, 0,        //
    1023, 0, 0,        // 3, ...
    547,  0, 0,        // 6, ...
    372,  0, 0, 0, 0,  // 9, ...
    273,  0, 0,        // 14, ...
    215,  0, 0,        // 17, ...
    178,  0, 0,        // 20, ...
    151,  0, 0,        // 23, ... (113 & 203 are base angles)
    132,  0, 0,        // 26, ...
    116,  0, 0,        // 29, ...
    102,  0, 0, 0,     // 32, ...
    90,   0, 0,        // 36, ...
    80,   0, 0,        // 39, ...
    71,   0, 0,        // 42, ...
    64,   0, 0,        // 45, ... (45 & 135 are base angles)
    57,   0, 0,        // 48, ...
    51,   0, 0,        // 51, ...
    45,   0, 0, 0,     // 54, ...
    40,   0, 0,        // 58, ...
    35,   0, 0,        // 61, ...
    31,   0, 0,        // 64, ...
    27,   0, 0,        // 67, ... (67 & 157 are base angles)
    23,   0, 0,        // 70, ...
    19,   0, 0,        // 73, ...
    15,   0, 0, 0, 0,  // 76, ...
    11,   0, 0,        // 81, ...
    7,    0, 0,        // 84, ...
    3,    0, 0,        // 87, ...
};

static INLINE uint16_t get_dy(int32_t angle) {
    if (angle > 90 && angle < 180) {
        return dr_intra_derivative[angle - 90];
    } else if (angle > 180 && angle < 270) {
        return dr_intra_derivative[270 - angle];
    } else {
        // In this case, we are not really going to use dy. We may return any
        // value.
        return 1;
    }
}
// Get the shift (up-scaled by 256) in X w.r.t a unit change in Y.
// If angle > 0 && angle < 90, dx = -((int32_t)(256 / t));
// If angle > 90 && angle < 180, dx = (int32_t)(256 / t);
// If angle > 180 && angle < 270, dx = 1;
static INLINE uint16_t get_dx(int32_t angle) {
    if (angle > 0 && angle < 90) {
        return dr_intra_derivative[angle];
    } else if (angle > 90 && angle < 180) {
        return dr_intra_derivative[180 - angle];
    } else {
        // In this case, we are not really going to use dx. We may return any
        // value.
        return 1;
    }
}

const int kZ1Start = 0;
const int kZ2Start = 90;
const int kZ3Start = 180;

const TxSize kTxSize[] = {TX_4X4,
                          TX_8X8,
                          TX_16X16,
                          TX_32X32,
                          TX_64X64,
                          TX_4X8,
                          TX_8X4,
                          TX_8X16,
                          TX_16X8,
                          TX_16X32,
                          TX_32X16,
                          TX_32X64,
                          TX_64X32,
                          TX_4X16,
                          TX_16X4,
                          TX_8X32,
                          TX_32X8,
                          TX_16X64,
                          TX_64X16};

const char *const kTxSizeStrings[] = {"TX_4X4",
                                      "TX_8X8",
                                      "TX_16X16",
                                      "TX_32X32",
                                      "TX_64X64",
                                      "TX_4X8",
                                      "TX_8X4",
                                      "TX_8X16",
                                      "TX_16X8",
                                      "TX_16X32",
                                      "TX_32X16",
                                      "TX_32X64",
                                      "TX_64X32",
                                      "TX_4X16",
                                      "TX_16X4",
                                      "TX_8X32",
                                      "TX_32X8",
                                      "TX_16X64",
                                      "TX_64X16"};

using svt_av1_test_tool::SVTRandom;

typedef void (*DrPred_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                           const uint16_t *above, const uint16_t *left,
                           int upsample_above, int upsample_left, int dx,
                           int dy, int bd);

typedef void (*DrPred)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_above, int upsample_left, int dx, int dy,
                       int bd);

typedef void (*Z1_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_above, int dx, int dy);
template <Z1_Lbd fn>
void z1_wrapper(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                const uint8_t *above, const uint8_t *left, int upsample_above,
                int /*upsample_left*/, int dx, int dy, int /*bd*/) {
    fn(dst, stride, bw, bh, above, left, upsample_above, dx, dy);
}

typedef void (*Z2_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_above, int upsample_left, int dx, int dy);
template <Z2_Lbd fn>
void z2_wrapper(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                const uint8_t *above, const uint8_t *left, int upsample_above,
                int upsample_left, int dx, int dy, int /*bd*/) {
    fn(dst, stride, bw, bh, above, left, upsample_above, upsample_left, dx, dy);
}

typedef void (*Z3_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_left, int dx, int dy);
template <Z3_Lbd fn>
void z3_wrapper(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                const uint8_t *above, const uint8_t *left,
                int /*upsample_above*/, int upsample_left, int dx, int dy,
                int /*bd*/) {
    fn(dst, stride, bw, bh, above, left, upsample_left, dx, dy);
}

typedef void (*Z1_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_above, int dx, int dy, int bd);
template <Z1_Hbd fn>
void z1_wrapper_hbd(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                    const uint16_t *above, const uint16_t *left,
                    int upsample_above, int /*upsample_left*/, int dx, int dy,
                    int bd) {
    fn(dst, stride, bw, bh, above, left, upsample_above, dx, dy, bd);
}

typedef void (*Z2_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_above, int upsample_left, int dx, int dy,
                       int bd);
template <Z2_Hbd fn>
void z2_wrapper_hbd(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                    const uint16_t *above, const uint16_t *left,
                    int upsample_above, int upsample_left, int dx, int dy,
                    int bd) {
    fn(dst,
       stride,
       bw,
       bh,
       above,
       left,
       upsample_above,
       upsample_left,
       dx,
       dy,
       bd);
}

typedef void (*Z3_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_left, int dx, int dy, int bd);
template <Z3_Hbd fn>
void z3_wrapper_hbd(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                    const uint16_t *above, const uint16_t *left,
                    int /*upsample_above*/, int upsample_left, int dx, int dy,
                    int bd) {
    fn(dst, stride, bw, bh, above, left, upsample_left, dx, dy, bd);
}

template <typename FuncType>
struct DrPredFunc {
    DrPredFunc(FuncType pred = NULL, FuncType tst = NULL,
               int bit_depth_value = 0, int start_angle_value = 0)
        : ref_fn(pred),
          tst_fn(tst),
          bit_depth(bit_depth_value),
          start_angle(start_angle_value) {
    }

    FuncType ref_fn;
    FuncType tst_fn;
    int bit_depth;
    int start_angle;
};

template <typename Pixel, typename FuncType>
class DrPredTest : public ::testing::TestWithParam<DrPredFunc<FuncType>> {
  protected:
    static const int kMaxNumTests = 100000;
    static const int kIterations = 10;
    static const int kDstStride = 64;
    static const int kDstSize = kDstStride * kDstStride;
    static const int kOffset = 16;
    static const int kBufSize = ((2 * MAX_TX_SIZE) << 1) + 16;

    DrPredTest()
        : upsample_above_(0),
          upsample_left_(0),
          bw_(0),
          bh_(0),
          dx_(1),
          dy_(1),
          bd_(8),
          txsize_(TX_4X4) {
        params_ = this->GetParam();
        start_angle_ = params_.start_angle;
        stop_angle_ = start_angle_ + 90;

        dst_ref_ = &dst_ref_data_[0];
        dst_tst_ = &dst_tst_data_[0];
        dst_stride_ = kDstStride;
        above_ = &above_data_[kOffset];
        left_ = &left_data_[kOffset];

        SVTRandom rnd(0, (1 << 8) - 1);
        for (int i = 0; i < kBufSize; ++i) {
            above_data_[i] = rnd.random();
            left_data_[i] = rnd.random();
        }

        for (int i = 0; i < kDstSize; ++i) {
            dst_ref_[i] = 0;
        }
    }

    virtual ~DrPredTest() {
    }

    void Predict(bool speedtest, int tx) {
        const int kNumTests = speedtest ? kMaxNumTests : 1;
        for (int k = 0; k < kNumTests; ++k) {
            params_.ref_fn(dst_ref_,
                           dst_stride_,
                           bw_,
                           bh_,
                           above_,
                           left_,
                           upsample_above_,
                           upsample_left_,
                           dx_,
                           dy_,
                           bd_);
        }
        if (params_.tst_fn) {
            for (int k = 0; k < kNumTests; ++k) {
                params_.tst_fn(dst_tst_,
                               dst_stride_,
                               bw_,
                               bh_,
                               above_,
                               left_,
                               upsample_above_,
                               upsample_left_,
                               dx_,
                               dy_,
                               bd_);
            }
        }
    }

    void RunTest(bool speedtest) {
        for (int i = 0; i < kBufSize; ++i) {
            above_data_[i] = left_data_[i] = (1 << bd_) - 1;
        }

        for (int tx = 0; tx < TX_SIZES_ALL; ++tx) {
            if (params_.tst_fn == NULL) {
                for (int i = 0; i < kDstSize; ++i) {
                    dst_tst_[i] = (1 << bd_) - 1;
                }
            } else {
                for (int i = 0; i < kDstSize; ++i) {
                    dst_tst_[i] = 0;
                }
            }

            bw_ = tx_size_wide[kTxSize[tx]];
            bh_ = tx_size_high[kTxSize[tx]];

            Predict(speedtest, tx);

            for (int r = 0; r < bh_; ++r) {
                for (int c = 0; c < bw_; ++c) {
                    ASSERT_EQ(dst_ref_[r * dst_stride_ + c],
                              dst_tst_[r * dst_stride_ + c])
                        << bw_ << "x" << bh_ << " r: " << r << " c: " << c
                        << " dx: " << dx_ << " dy: " << dy_
                        << " upsample_above: " << upsample_above_
                        << " upsample_left: " << upsample_left_;
                }
            }
        }
    }

    Pixel dst_ref_data_[kDstSize];
    Pixel dst_tst_data_[kDstSize];

    Pixel left_data_[kBufSize];
    Pixel dummy_data_[kBufSize];
    Pixel above_data_[kBufSize];

    Pixel *dst_ref_;
    Pixel *dst_tst_;
    Pixel *above_;
    Pixel *left_;
    int dst_stride_;

    int upsample_above_;
    int upsample_left_;
    int bw_;
    int bh_;
    int dx_;
    int dy_;
    int bd_;
    TxSize txsize_;

    int start_angle_;
    int stop_angle_;

    DrPredFunc<FuncType> params_;
};

class LowbdDrPredTest : public DrPredTest<uint8_t, DrPred> {};

TEST_P(LowbdDrPredTest, SaturatedValues) {
    for (int iter = 0; iter < kIterations && !HasFatalFailure(); ++iter) {
        upsample_above_ = iter & 1;
        for (int angle = start_angle_; angle < stop_angle_; ++angle) {
            dx_ = get_dx(angle);
            dy_ = get_dy(angle);
            if (dx_ && dy_)
                RunTest(false);
        }
    }
}

TEST_P(LowbdDrPredTest, DISABLED_Speed) {
    const int angles[] = {3, 45, 87};
    for (upsample_above_ = 0; upsample_above_ < 2; ++upsample_above_) {
        upsample_left_ = upsample_above_;
        for (int i = 0; i < 3; ++i) {
            dx_ = get_dx(angles[i] + start_angle_);
            dy_ = get_dy(angles[i] + start_angle_);
            printf(
                "upsample_above: %d upsample_left: %d angle: %d "
                "~~~~~~~~~~~~~~~\n",
                upsample_above_,
                upsample_left_,
                angles[i] + start_angle_);
            if (dx_ && dy_)
                RunTest(true);
        }
    }
}

using std::make_tuple;

INSTANTIATE_TEST_CASE_P(
    C, LowbdDrPredTest,
    ::testing::Values(DrPredFunc<DrPred>(&z1_wrapper<av1_dr_prediction_z1_c>,
                                         &z1_wrapper<av1_dr_prediction_z1_avx2>,
                                         AOM_BITS_8, kZ1Start),
                      DrPredFunc<DrPred>(&z2_wrapper<av1_dr_prediction_z2_c>,
                                         &z2_wrapper<av1_dr_prediction_z2_avx2>,
                                         AOM_BITS_8, kZ2Start),
                      DrPredFunc<DrPred>(&z3_wrapper<av1_dr_prediction_z3_c>,
                                         &z3_wrapper<av1_dr_prediction_z3_avx2>,
                                         AOM_BITS_8, kZ3Start)));

class HighbdDrPredTest : public DrPredTest<uint16_t, DrPred_Hbd> {};

TEST_P(HighbdDrPredTest, SaturatedValues) {
    for (int iter = 0; iter < kIterations && !HasFatalFailure(); ++iter) {
        upsample_above_ = iter & 1;
        for (int angle = start_angle_; angle < stop_angle_; ++angle) {
            dx_ = get_dx(angle);
            dy_ = get_dy(angle);
            if (dx_ && dy_)
                RunTest(false);
        }
    }
}

TEST_P(HighbdDrPredTest, DISABLED_Speed) {
    const int angles[] = {3, 45, 87};
    for (upsample_above_ = 0; upsample_above_ < 2; ++upsample_above_) {
        upsample_left_ = upsample_above_;
        for (int i = 0; i < 3; ++i) {
            dx_ = get_dx(angles[i] + start_angle_);
            dy_ = get_dy(angles[i] + start_angle_);
            printf(
                "upsample_above: %d upsample_left: %d angle: %d "
                "~~~~~~~~~~~~~~~\n",
                upsample_above_,
                upsample_left_,
                angles[i] + start_angle_);
            if (dx_ && dy_)
                RunTest(true);
        }
    }
}

INSTANTIATE_TEST_CASE_P(
    C, HighbdDrPredTest,
    ::testing::Values(DrPredFunc<DrPred_Hbd>(
                          &z1_wrapper_hbd<av1_highbd_dr_prediction_z1_c>,
                          &z1_wrapper_hbd<av1_highbd_dr_prediction_z1_avx2>,
                          AOM_BITS_10, kZ1Start),
                      DrPredFunc<DrPred_Hbd>(
                          &z2_wrapper_hbd<av1_highbd_dr_prediction_z2_c>,
                          &z2_wrapper_hbd<av1_highbd_dr_prediction_z2_avx2>,
                          AOM_BITS_10, kZ2Start),
                      DrPredFunc<DrPred_Hbd>(
                          &z3_wrapper_hbd<av1_highbd_dr_prediction_z3_c>,
                          &z3_wrapper_hbd<av1_highbd_dr_prediction_z3_avx2>,
                          AOM_BITS_10, kZ3Start)));

}  // namespace
