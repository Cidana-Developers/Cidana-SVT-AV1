/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file intrapred_dr_test.cc
 *
 * @brief Unit test for intra directional prediction :
 * - av1_highbd_dr_prediction_z{1, 2, 3}_avx2
 * - av1_dr_prediction_z{1, 2, 3}_avx2
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/

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

using svt_av1_test_tool::SVTRandom;

typedef void (*Z1_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_above, int dx, int dy);

typedef void (*Z2_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_above, int upsample_left, int dx, int dy);

typedef void (*Z3_Lbd)(uint8_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint8_t *above, const uint8_t *left,
                       int upsample_left, int dx, int dy);

typedef void (*Z1_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_above, int dx, int dy, int bd);

typedef void (*Z2_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_above, int upsample_left, int dx, int dy,
                       int bd);
typedef void (*Z3_Hbd)(uint16_t *dst, ptrdiff_t stride, int bw, int bh,
                       const uint16_t *above, const uint16_t *left,
                       int upsample_left, int dx, int dy, int bd);

template <typename Pixel, typename FuncType>
class DrPredTest {
  public:
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
          dy_(1) {
        dst_ref_ = &dst_ref_data_[0];
        dst_tst_ = &dst_tst_data_[0];
        dst_stride_ = kDstStride;
        above_ = &above_data_[kOffset];
        left_ = &left_data_[kOffset];

    }

    virtual ~DrPredTest() {
    }

    void RunAllTest() {
        for (int angle = start_angle_; angle < stop_angle_; ++angle) {
            dx_ = get_dx(angle);
            dy_ = get_dy(angle);
            if (dx_ & dy_)
                RunTest();
        }
    }

  protected:
    virtual void Predict(){};

    void prepare_neighbor_pixel(SVTRandom &rnd, int cnt) {
        if (cnt == 0) {
            for (int i = 0; i < kBufSize; ++i)
                above_data_[i] = left_data_[i] = (1 << bd_) - 1;
        } else {
            for (int i = 0; i < kBufSize; ++i) {
                above_data_[i] = rnd.random();
                left_data_[i] = rnd.random();
            }
        }
    }

    void clear_output_pixel() {
        for (int i = 0; i < kDstSize; ++i) {
            dst_ref_[i] = 0;
        }

        for (int i = 0; i < kDstSize; ++i) {
            dst_tst_[i] = 0;
        }
    }

    void RunTest() {
        SVTRandom rnd(0, (1 << bd_) - 1);
        for (int cnt = 0; cnt < kIterations; ++cnt) {
            prepare_neighbor_pixel(rnd, cnt);
            for (int tx = 0; tx < TX_SIZES_ALL; ++tx) {
                // clear the output for each tx
                clear_output_pixel();

                // TODO(wenyao): loop upsample_above and upsample_left

                bw_ = tx_size_wide[tx];
                bh_ = tx_size_high[tx];
                Predict();

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
    }

    Pixel dst_ref_data_[kDstSize];
    Pixel dst_tst_data_[kDstSize];

    Pixel left_data_[kBufSize];
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

    int start_angle_;
    int stop_angle_;

    FuncType ref_fn;
    FuncType tst_fn;
};

class LowbdZ1PredTest : public DrPredTest<uint8_t, Z1_Lbd> {
  public:
    LowbdZ1PredTest() {
        start_angle_ = kZ1Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_dr_prediction_z1_c;
        tst_fn = av1_dr_prediction_z1_avx2;
        bd_ = 8;
        DrPredTest();
    }

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               dx_,
               dy_);
        tst_fn(dst_tst_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               dx_,
               dy_);
    }
};

class LowbdZ2PredTest : public DrPredTest<uint8_t, Z2_Lbd> {
  public:
    LowbdZ2PredTest() {
        start_angle_ = kZ2Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_dr_prediction_z2_c;
        tst_fn = av1_dr_prediction_z2_avx2;
        bd_ = 8;
        DrPredTest();
    }

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               upsample_left_,
               dx_,
               dy_);
        tst_fn(dst_tst_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               upsample_left_,
               dx_,
               dy_);
    }
};

class LowbdZ3PredTest : public DrPredTest<uint8_t, Z3_Lbd> {
  public:
    LowbdZ3PredTest() {
        start_angle_ = kZ3Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_dr_prediction_z3_c;
        tst_fn = av1_dr_prediction_z3_avx2;
        bd_ = 8;

        DrPredTest();
    };

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_left_,
               dx_,
               dy_);
        tst_fn(dst_tst_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_left_,
               dx_,
               dy_);
    }
};

#define TEST_CLASS(tc_name, type_name)        \
    TEST(tc_name, match_test) {               \
        type_name *dr_test = new type_name(); \
        dr_test->RunAllTest();                \
        delete dr_test;                       \
    }

TEST_CLASS(LowbdDrZ1Test, LowbdZ1PredTest)
TEST_CLASS(LowbdDrZ2Test, LowbdZ2PredTest)
TEST_CLASS(LowbdDrZ3Test, LowbdZ3PredTest)

class HighbdZ1PredTest : public DrPredTest<uint16_t, Z1_Hbd> {
  public:
    HighbdZ1PredTest() {
        start_angle_ = kZ1Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_highbd_dr_prediction_z1_c;
        tst_fn = av1_highbd_dr_prediction_z1_avx2;
        bd_ = 10;

        DrPredTest();
    };

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               dx_,
               dy_,
               bd_);
        tst_fn(dst_tst_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_above_,
               dx_,
               dy_,
               bd_);
    }
};

class HighbdZ2PredTest : public DrPredTest<uint16_t, Z2_Hbd> {
  public:
    HighbdZ2PredTest() {
        start_angle_ = kZ2Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_highbd_dr_prediction_z2_c;
        tst_fn = av1_highbd_dr_prediction_z2_avx2;
        bd_ = 10;

        DrPredTest();
    };

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
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
        tst_fn(dst_tst_,
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
};

class HighbdZ3PredTest : public DrPredTest<uint16_t, Z3_Hbd> {
  public:
    HighbdZ3PredTest() {
        start_angle_ = kZ3Start;
        stop_angle_ = start_angle_ + 90;
        ref_fn = av1_highbd_dr_prediction_z3_c;
        tst_fn = av1_highbd_dr_prediction_z3_avx2;
        bd_ = 10;

        DrPredTest();
    };

  protected:
    void Predict() override {
        ref_fn(dst_ref_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_left_,
               dx_,
               dy_,
               bd_);
        tst_fn(dst_tst_,
               dst_stride_,
               bw_,
               bh_,
               above_,
               left_,
               upsample_left_,
               dx_,
               dy_,
               bd_);
    }
};

TEST_CLASS(HighbdDrZ1Test, HighbdZ1PredTest)
TEST_CLASS(HighbdDrZ2Test, HighbdZ2PredTest)
TEST_CLASS(HighbdDrZ3Test, HighbdZ3PredTest)

}  // namespace
