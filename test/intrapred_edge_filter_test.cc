/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file intrapred_edge_filter_test.cc
 *
 * @brief Unit test for upsample and edge filter:
 * - av1_upsample_intra_edge_sse4_1
 * - av1_filter_intra_edge_sse4_1
 * - av1_filter_intra_edge_high_sse4_1
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/

#include "gtest/gtest.h"
#include "aom_dsp_rtcd.h"
#include "EbDefinitions.h"
#include "random.h"

namespace {
using svt_av1_test_tool::SVTRandom;

// -----------------------------------------------------------------------------
// Upsample test
using UPSAMPLE_LBD = void (*)(uint8_t *p, int size);

/**
 * @brief Unit test for upsample in intra prediction:
 * - av1_upsample_intra_edge_sse4_1
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check test output and reference output.
 * Define a templete class to handle the common process, and
 * declare sub class to handle different bitdepth and function types.
 *
 * Expect result:
 * Output from assemble functions should be the same with output from c.
 *
 * Test coverage:
 * Test cases:
 * Neighbor pixel buffer: Fill with random values
 * Size: [4, 16]
 * BitDepth: 8bit
 */
template <typename Pixel, typename Func>
class UpsampleTest {
  public:
    UpsampleTest() {
        ref_func_ = nullptr;
        tst_func_ = nullptr;
        bd_ = 8;
        common_init();
    }

    virtual ~UpsampleTest() {
    }

    void RunTest() {
        SVTRandom pix_rnd(0, (1 << bd_) - 1);
        for (int iter = 0; iter < num_tests; ++iter) {
            for (int i = 1; i < 5; ++i) {  // [1, 4]
                size_ = 4 * i;
                prepare_data(pix_rnd);

                run_upsample();

                const int max_idx = (size_ - 1) * 2;
                for (int i = -2; i <= max_idx; ++i)
                    ASSERT_EQ(edge_ref_[i], edge_tst_[i]);
            }
        }
    }

  protected:
    static const int num_tests = 1000000;
    static const int edge_buf_size = 2 * 64 + 32;
    static const int start_offset = 16;

    void common_init() {
        edge_ref_ = &edge_ref_data_[start_offset];
        edge_tst_ = &edge_tst_data_[start_offset];
    }

    void prepare_data(SVTRandom &pix_rnd) {
        int i = 0;
        for (; i < start_offset + size_; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = pix_rnd.random();
        }

        Pixel last = edge_ref_data_[start_offset + size_ - 1];
        for (; i < edge_buf_size; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = last;
        }
    }

    virtual void run_upsample() {
    }

    Pixel edge_ref_data_[edge_buf_size];
    Pixel edge_tst_data_[edge_buf_size];

    Pixel *edge_ref_;
    Pixel *edge_tst_;

    Func ref_func_;
    Func tst_func_;
    int size_;
    int bd_;
};

class LowbdUpsampleTest : public UpsampleTest<uint8_t, UPSAMPLE_LBD> {
  public:
    LowbdUpsampleTest() {
        ref_func_ = av1_upsample_intra_edge_c;
        tst_func_ = av1_upsample_intra_edge_sse4_1;
        bd_ = 8;
        common_init();
    }

  protected:
    void run_upsample() override {
        ref_func_(edge_ref_, size_);
        tst_func_(edge_tst_, size_);
    }
};

#define TEST_CLASS(tc_name, type_name)     \
    TEST(tc_name, match_test) {            \
        type_name *test = new type_name(); \
        test->RunTest();                   \
        delete test;                       \
    }

TEST_CLASS(UpsampleTestLBD, LowbdUpsampleTest)

// -----------------------------------------------------------------------------
// Filter edge Tests
// Declare macros and functions requried
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

using FILTER_EDGE_LBD = void (*)(uint8_t *p, int size, int strength);
using FILTER_EDGE_HBD = void (*)(uint16_t *p, int size, int strength);

/**
 * @brief Unit test for edge filter in intra prediction:
 * - av1_filter_intra_edge_sse4_1
 * - av1_filter_intra_edge_high_sse4_1
 *
 * Test strategy:
 * Verify this assembly code by comparing with reference c implementation.
 * Feed the same data and check test output and reference output.
 * Define a templete class to handle the common process, and
 * declare sub class to handle different bitdepth and function types.
 *
 * Expect result:
 * Output from assemble functions should be the same with output from c.
 *
 * Test coverage:
 * Test cases:
 * Neighbor pixel buffer: Fill with random values
 * Strength: [0, 3]
 * Size: [5, 129]
 * BitDepth: 8bit and 10bit
 */
template <typename Pixel, typename Func>
class FilterEdgeTest {
  public:
    FilterEdgeTest() {
        ref_func_ = tst_func_ = nullptr;
        bd_ = 8;
        common_init();
    }

    virtual ~FilterEdgeTest() {
    }

    void RunTest() {
        SVTRandom size_rnd(1, 32);     // range [1, 32]
        SVTRandom strength_rnd(0, 3);  // range [0, 3]
        SVTRandom pix_rnd(0, (1 << bd_) - 1);
        for (int iter = 0; iter < num_tests; ++iter) {
            // random strength and size
            strength_ = strength_rnd.random();
            size_ = 4 * size_rnd.random() + 1;

            prepare_data(pix_rnd);

            run_filter_edge();

            for (int i = 0; i < size_; ++i)
                ASSERT_EQ(edge_ref_[i], edge_tst_[i]);
        }
    }

  protected:
    static const int num_tests = 1000000;
    static const int edge_buf_size = 2 * 64 + 32;
    static const int start_offset = 15;

    void common_init() {
        edge_ref_ = &edge_ref_data_[start_offset];
        edge_tst_ = &edge_tst_data_[start_offset];
    }

    void prepare_data(SVTRandom &pix_rnd) {
        int i = 0;
        for (; i < start_offset + size_; ++i) {
            edge_ref_data_[i] = edge_tst_data_[i] = pix_rnd.random();
        }
    }

    void run_filter_edge() {
        ref_func_(edge_ref_, size_, strength_);
        tst_func_(edge_tst_, size_, strength_);
    }

    Pixel edge_ref_data_[edge_buf_size];
    Pixel edge_tst_data_[edge_buf_size];

    Pixel *edge_ref_;
    Pixel *edge_tst_;

    Func ref_func_;
    Func tst_func_;
    int size_;
    int bd_;
    int strength_;
};

class LowbdFilterEdgeTest : public FilterEdgeTest<uint8_t, FILTER_EDGE_LBD> {
  public:
    LowbdFilterEdgeTest() {
        ref_func_ = av1_filter_intra_edge_c;
        tst_func_ = av1_filter_intra_edge_sse4_1;
        bd_ = 8;
        common_init();
    }
};

class HighbdFilterEdgeTest : public FilterEdgeTest<uint16_t, FILTER_EDGE_HBD> {
  public:
    HighbdFilterEdgeTest() {
        ref_func_ = av1_filter_intra_edge_high_c;
        tst_func_ = av1_filter_intra_edge_high_sse4_1;
        bd_ = 10;
        common_init();
    }
};

TEST_CLASS(IntraFilterEdgeTestLowbd, LowbdFilterEdgeTest)
TEST_CLASS(IntraFilterEdgeTestHighbd, HighbdFilterEdgeTest)
}  // namespace
