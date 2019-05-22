/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file intrapred_edge_filter_test.cc
 *
 * @brief Unit test for upsample and filter in spec :
 * - av1_upsample_intra_edge_sse4_1
 * - av1_filter_intra_edge_sse4_1
 * - av1_filter_intra_edge_high_sse4_1
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/
#include <string>

#include "gtest/gtest.h"

#include "aom_dsp_rtcd.h"
#include "EbDefinitions.h"
#include "random.h"

using UPSAMPLE_LOWBD = void (*)(uint8_t *p, int size);
using svt_av1_test_tool::SVTRandom;

template <typename Pixel, typename Func>
class UpsampleTest {
  public:
    UpsampleTest() {
        edge_ref_ = &edge_ref_data_[kOffset];
        edge_tst_ = &edge_tst_data_[kOffset];
    }

    virtual ~UpsampleTest() {
    }

    void RunTest() {
        SVTRandom rnd(1, 4);
        SVTRandom pix_rnd(0, (1 << bd_) - 1);
        for (int iter = 0; iter < kIterations; ++iter) {
            size_ = 4 * rnd.random();  // TODO: how to set size_, is there any
                                       // better way
            prepare_data(pix_rnd);

            run_func_pair();

            const int max_idx = (size_ - 1) * 2;
            for (int i = -2; i <= max_idx; ++i)
                ASSERT_EQ(edge_ref_[i], edge_tst_[i]);
        }
    }

  protected:
    static const int kIterations = 1000000;
    static const int kMinEdge = 4;
    static const int kMaxEdge = 24;
    static const int kBufSize = 2 * 64 + 32;
    static const int kOffset = 16;

    void prepare_data(SVTRandom &pix_rnd) {
        int i = 0;
        for (; i < kOffset + size_; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = pix_rnd.random();
        }

        Pixel last = edge_ref_data_[kOffset + size_ - 1];
        for (; i < kBufSize; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = last;
        }
    }

    virtual void run_func_pair(){};
    Pixel edge_ref_data_[kBufSize];
    Pixel edge_tst_data_[kBufSize];

    Pixel *edge_ref_;
    Pixel *edge_tst_;

    Func ref_func_;
    Func test_func_;
    int size_;
    int bd_;
};

class LowbdUpsampleTest : public UpsampleTest<uint8_t, UPSAMPLE_LOWBD> {
  public:
    LowbdUpsampleTest() {
        ref_func_ = av1_upsample_intra_edge_c;
        test_func_ = av1_upsample_intra_edge_sse4_1;
        bd_ = 8;
        UpsampleTest();
    }

  protected:
    void run_func_pair() override {
        ref_func_(edge_ref_, size_);
        test_func_(edge_tst_, size_);
    }
};

#define TEST_CLASS(tc_name, type_name)     \
    TEST(tc_name, match_test) {            \
        type_name *test = new type_name(); \
        test->RunTest();                   \
        delete test;                       \
    }

TEST_CLASS(UpsampleTestLB, LowbdUpsampleTest)

#define INTRA_EDGE_FILT 3
#define INTRA_EDGE_TAPS 5
#define MAX_UPSAMPLE_SZ 16
static void av1_filter_intra_edge_c(uint8_t *p, int sz, int strength) {
    if (!strength)
        return;

    const int kernel[INTRA_EDGE_FILT][INTRA_EDGE_TAPS] = {
        {0, 4, 8, 4, 0}, {0, 5, 6, 5, 0}, {2, 4, 4, 4, 2}};
    const int filt = strength - 1;
    uint8_t edge[129];

    memcpy(edge, p, sz * sizeof(*p));
    for (int i = 1; i < sz; i++) {
        int s = 0;
        for (int j = 0; j < INTRA_EDGE_TAPS; j++) {
            int k = i - 2 + j;
            k = (k < 0) ? 0 : k;
            k = (k > sz - 1) ? sz - 1 : k;
            s += edge[k] * kernel[filt][j];
        }
        s = (s + 8) >> 4;
        p[i] = s;
    }
}

using FILTER_EDGE_LOWBD = void (*)(uint8_t *p, int size, int strength);
using FILTER_EDGE_HIGHBD = void (*)(uint16_t *p, int size, int strength);
template <typename Pixel, typename Func>
class FilterEdgeTest {
  public:
    FilterEdgeTest() {
        edge_ref_ = &edge_ref_data_[kOffset];
        edge_tst_ = &edge_tst_data_[kOffset];
    }

    virtual ~FilterEdgeTest() {
    }

    void RunTest() {
        SVTRandom rnd(1, 32);
        SVTRandom rnd1(0, 3);
        SVTRandom pix_rnd(0, (1 << bd_) - 1);
        for (int iter = 0; iter < kIterations; ++iter) {
            strength_ = rnd1.random();  // TODO: refine the random
            size_ =
                4 * rnd.random() + 1;  // TODO: how to set size_, is there any
                                       // better way
            prepare_data(pix_rnd);

            run_func_pair();

            for (int i = 0; i < size_; ++i)
                ASSERT_EQ(edge_ref_[i], edge_tst_[i]);
        }
    }

  protected:
    static const int kIterations = 1000000;
    static const int kMinEdge = 4;
    static const int kMaxEdge = 2 * 64;
    static const int kBufSize = 2 * 64 + 32;
    static const int kOffset = 15;

    void prepare_data(SVTRandom &pix_rnd) {
        int i = 0;
        for (; i < kOffset + size_; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = pix_rnd.random();
        }
    }

    virtual void run_func_pair(){};

    Pixel edge_ref_data_[kBufSize];
    Pixel edge_tst_data_[kBufSize];

    Pixel *edge_ref_;
    Pixel *edge_tst_;

    Func ref_func_;
    Func test_func_;
    int size_;
    int bd_;
    int strength_;
};

class LowbdFilterEdgeTest : public FilterEdgeTest<uint8_t, FILTER_EDGE_LOWBD> {
  public:
    LowbdFilterEdgeTest() {
        ref_func_ = av1_filter_intra_edge_c;
        test_func_ = av1_filter_intra_edge_sse4_1;
        bd_ = 8;
        FilterEdgeTest();
    }

  protected:
    void run_func_pair() override {
        ref_func_(edge_ref_, size_, strength_);
        test_func_(edge_tst_, size_, strength_);
    }
};

class HighbdFilterEdgeTest
    : public FilterEdgeTest<uint16_t, FILTER_EDGE_HIGHBD> {
  public:
    HighbdFilterEdgeTest() {
        ref_func_ = av1_filter_intra_edge_high_c;
        test_func_ = av1_filter_intra_edge_high_sse4_1;
        bd_ = 10;
        FilterEdgeTest();
    }

  protected:
    void run_func_pair() override {
        ref_func_(edge_ref_, size_, strength_);
        test_func_(edge_tst_, size_, strength_);
    }
};

TEST_CLASS(IntraFilterEdgeTestLowbd, LowbdFilterEdgeTest)
TEST_CLASS(IntraFilterEdgeTestHighbd, HighbdFilterEdgeTest)
