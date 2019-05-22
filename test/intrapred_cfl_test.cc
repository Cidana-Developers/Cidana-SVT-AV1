/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file intrapred_edge_filter_test.cc
 *
 * @brief Unit test for upsample and filter in spec :
 * - cfl_predict_hbd_avx2
 * - cfl_predict_lbd_avx2
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/

// calculate alpha_q3 by cfl_idx_to_alpha
// cfl_predict_hbd
// do a 64x64
// pred_buf_q3 + 1, bitdepth + 3 + 1, signed
// alpha: 0 ~ 15
// tx uv
#include <string>

#include "gtest/gtest.h"

#include "aom_dsp_rtcd.h"
#include "EbDefinitions.h"
#include "random.h"

using svt_av1_test_tool::SVTRandom;

using CFL_PRED_HBD = void (*)(const int16_t *pred_buf_q3, uint16_t *pred,
                              int32_t pred_stride, uint16_t *dst,
                              int32_t dst_stride, int32_t alpha_q3,
                              int32_t bit_depth, int32_t width, int32_t height);
using CFL_PRED_LBD = void (*)(const int16_t *pred_buf_q3, uint8_t *pred,
                              int32_t pred_stride, uint8_t *dst,
                              int32_t dst_stride, int32_t alpha_q3,
                              int32_t bit_depth, int32_t width, int32_t height);
template <typename Pixel, typename FuncType>
class CflPredTest {
  public:
    CflPredTest() {
        dst_buf_ref_ =
            (Pixel *)(((intptr_t)(dst_buf_ref_data_) + alignment - 1) &
                      ~(alignment - 1));
        dst_buf_tst_ =
            (Pixel *)(((intptr_t)(dst_buf_tst_data_) + alignment - 1) &
                      ~(alignment - 1));
    }

    virtual ~CflPredTest() {
    }

    void print_result(const char *message, Pixel *buf, const int c_w, const int c_h, const int c_stride){
        printf("%s %dx%d\n", message, c_w, c_h);
        for(int i = 0; i < c_h; i++){
            for(int j = 0; j < c_w; j++){
                printf("%d ", buf[i * c_stride + j]);
            }
            printf("\n");
        }
        printf("\n");
    }


    void RunAllTest() {
        // for pred_buf, after sampling and subtracted from average
        SVTRandom pred_rnd(bd_ + 3 + 1, true);
        SVTRandom dst_rnd(4, false);
        for (int tx = TX_4X4; tx < TX_SIZES_ALL; ++tx) {
            // loop the alpha_q3, [-16, 16]
            // TODO(wenyao): support more color space
            const int c_w = tx_size_wide[tx] >> 1;
            const int c_h = tx_size_high[tx] >> 1;
            const int c_stride = CFL_BUF_LINE;
            memset(pred_buf_q3, 0, sizeof(pred_buf_q3));
            memset(dst_buf_ref_data_, 0, sizeof(dst_buf_ref_data_));
            memset(dst_buf_tst_data_, 0, sizeof(dst_buf_tst_data_));

            for (int alpha_q3 = -16; alpha_q3 <= 16; ++alpha_q3) {
                // prepare data
                for (int y = 0; y < c_h; ++y) {
                    for (int x = 0; x < c_w; ++x) {
                        pred_buf_q3[y * c_stride + x] = pred_rnd.random();
                        dst_buf_ref_[y * c_stride + x] =
                            dst_buf_tst_[y * c_stride + x] = dst_rnd.random();
                    }
                }

                print_result("before ref execution", dst_buf_ref_, c_w, c_h, c_stride);
                ref_func(pred_buf_q3,
                         dst_buf_ref_,
                         CFL_BUF_LINE,
                         dst_buf_ref_,
                         CFL_BUF_LINE,
                         alpha_q3,
                         bd_,
                         c_w,
                         c_h);
                print_result("after ref execution", dst_buf_ref_, c_w, c_h, c_stride);
                print_result("before tst execution", dst_buf_tst_, c_w, c_h, c_stride);
                tst_func(pred_buf_q3,
                         dst_buf_tst_,
                         c_stride,
                         dst_buf_tst_,
                         c_stride,
                         alpha_q3,
                         bd_,
                         c_w,
                         c_h);
                print_result("after tst execution", dst_buf_tst_, c_w, c_h, c_stride);

                for (int y = 0; y < c_h; ++y)
                    for (int x = 0; x < c_w; ++x) {
                        ASSERT_EQ(dst_buf_ref_[y * c_stride + x],
                                  dst_buf_tst_[y * c_stride + x])
                            << "tx_size: " << tx << " alpha_q3 " << alpha_q3
                            << " expect " << dst_buf_ref_[y * c_stride + x]
                            << " got " << dst_buf_tst_[y * c_stride + x]
                            << " at [ " << x << " x " << y << " ]";
                    }
            }
        }
    }

  protected:
    static const int alignment = 32;
    int16_t pred_buf_q3[CFL_BUF_SQUARE];
    Pixel dst_buf_ref_data_[CFL_BUF_SQUARE + alignment - 1];
    Pixel dst_buf_tst_data_[CFL_BUF_SQUARE + alignment - 1];
    Pixel *dst_buf_ref_;
    Pixel *dst_buf_tst_;
    FuncType ref_func;
    FuncType tst_func;
    int bd_;
};

class LbdCflPredTest : public CflPredTest<uint8_t, CFL_PRED_LBD> {
  public:
    LbdCflPredTest() {
        bd_ = 8;
        ref_func = cfl_predict_lbd_c;
        tst_func = cfl_predict_lbd_avx2;
        CflPredTest();
    }
};

class HbdCflPredTest : public CflPredTest<uint16_t, CFL_PRED_HBD> {
  public:
    HbdCflPredTest() {
        bd_ = 10;
        ref_func = cfl_predict_hbd_c;
        tst_func = cfl_predict_hbd_avx2;
        CflPredTest();
    }
};

#define TEST_CLASS(tc_name, type_name)     \
    TEST(tc_name, match_test) {            \
        type_name *test = new type_name(); \
        test->RunAllTest();                \
        delete test;                       \
    }

TEST_CLASS(LbdCflPredMatchTest, LbdCflPredTest)
TEST_CLASS(HbdCflPredMatchTest, HbdCflPredTest)
