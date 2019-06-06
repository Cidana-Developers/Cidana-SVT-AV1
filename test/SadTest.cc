
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "gtest/gtest.h"
#include "util.h"
#include "EbComputeSAD.h"
#include "random.h"

using svt_av1_test_tool::SVTRandom;  // to generate the random
namespace {
#define MAX_BLOCK_SIZE (128 * 256)
typedef std::tuple<int, int> SadResolutionParam;
typedef enum SadTestVectorParam {
    MaxRef,
    MaxSrc,
    Random,
    Unalign
} SadTestVectorParam;

SadResolutionParam resolutions_all[] = {SadResolutionParam(64, 64),
                                        SadResolutionParam(64, 32),
                                        SadResolutionParam(32, 64),
                                        SadResolutionParam(32, 32),
                                        SadResolutionParam(32, 16),
                                        SadResolutionParam(16, 32),
                                        SadResolutionParam(16, 16),
                                        SadResolutionParam(16, 8),
                                        SadResolutionParam(8, 16),
                                        SadResolutionParam(8, 8),
                                        SadResolutionParam(8, 4),
                                        SadResolutionParam(4, 8),
                                        SadResolutionParam(4, 16),
                                        SadResolutionParam(16, 4),
                                        SadResolutionParam(8, 32),
                                        SadResolutionParam(32, 8),
                                        SadResolutionParam(16, 64),
                                        SadResolutionParam(64, 16)};

SadTestVectorParam vectors[] = {MaxRef, MaxSrc, Random, Unalign};

typedef std::tuple<SadTestVectorParam, SadResolutionParam> SadTestParam;

class SADTestBase : public ::testing::Test {
  public:
    SADTestBase(const int width, const int height,
                SadTestVectorParam vector_param) {
        printf("SADTestBase\r\n");
        width_ = width;
        height_ = height;
        source_stride_ = ref_1_stride_ = ref_2_stride_ = width_ * 2;

        int32_t mask = (1 << 8) - 1;
        SVTRandom *rnd = new SVTRandom(0, mask);
        switch (vector_param) {
        case MaxRef: {
            for (int i = 0; i < MAX_BLOCK_SIZE; i++) {
                source_data_[i] = 0;
                ref_1_data_[i] = ref_2_data_[i] = mask;
            }
            break;
        }
        case MaxSrc: {
            for (int i = 0; i < MAX_BLOCK_SIZE; i++) {
                source_data_[i] = mask;
                ref_1_data_[i] = ref_2_data_[i] = 0;
            }
            break;
        }
        case Random: {
            for (int i = 0; i < MAX_BLOCK_SIZE; i++) {
                source_data_[i] = rnd->random();
                ref_1_data_[i] = rnd->random();
                ref_2_data_[i] = rnd->random();
            }
        } break;
        case Unalign: {
            for (int i = 0; i < MAX_BLOCK_SIZE; i++) {
                source_data_[i] = rnd->random();
                ref_1_data_[i] = rnd->random();
                ref_2_data_[i] = rnd->random();
            }
            source_stride_ -= 1;
            ref_1_stride_ -= 1;
            ref_2_stride_ -= 1;
        } break;
        default: break;
        }
        delete rnd;
    };

    uint32_t reference_sad() {
        unsigned int sad = 0;
        for (int h = 0; h < height_; ++h) {
            for (int w = 0; w < width_; ++w) {
                sad += abs(source_data_[h * source_stride_ + w] -
                           ref_1_data_[h * ref_1_stride_ + w]);
            }
        }
        return sad;
    }

    unsigned int reference_sad_avg() {
        unsigned int sad = 0;
        for (int h = 0; h < height_; ++h) {
            for (int w = 0; w < width_; ++w) {
                const int tmp = ref_2_data_[h * ref_2_stride_ + w] +
                                ref_1_data_[h * ref_1_stride_ + w];
                const uint8_t comp_pred = ROUND_POWER_OF_TWO(tmp, 1);
                sad += abs(source_data_[h * source_stride_ + w] - comp_pred);
            }
        }
        return sad;
    }

  protected:
    int width_, height_;
    int source_stride_;
    int ref_1_stride_;
    int ref_2_stride_;

    DECLARE_ALIGNED(32, uint8_t, source_data_[MAX_BLOCK_SIZE]);
    DECLARE_ALIGNED(32, uint8_t, ref_1_data_[MAX_BLOCK_SIZE]);
    DECLARE_ALIGNED(32, uint8_t, ref_2_data_[MAX_BLOCK_SIZE]);
};

class SADTestSubSample : public ::testing::WithParamInterface<SadTestParam>,
                         public SADTestBase {
  public:
    SADTestSubSample()
        : SADTestBase(std::get<0>(TEST_GET_PARAM(1)),
                      std::get<1>(TEST_GET_PARAM(1)), TEST_GET_PARAM(0)) {
    }

  protected:
    void check_sad() {
        EB_SADKERNELNxM_TYPE non_avx2_func = nullptr;
        EB_SADKERNELNxM_TYPE avx2_func = nullptr;
        switch (width_) {
        case 64: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][8];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][8];
            break;
        }
        case 48: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][6];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][6];
            break;
        }
        case 32: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][4];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][4];
            break;
        }
        case 24: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][3];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][3];
            break;
        }
        case 16: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][2];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][2];
            break;
        }
        case 8: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][1];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][1];
            break;
        }
        case 4: {
            non_avx2_func =
                NxMSadKernelSubSampled_funcPtrArray[ASM_NON_AVX2][0];
            avx2_func = NxMSadKernelSubSampled_funcPtrArray[ASM_AVX2][0];
            break;
        }
        default: break;
        }
        ASSERT_NE(non_avx2_func, nullptr)
            << "Missing non_avx2_func!" << width_ << " " << height_;
        ASSERT_NE(avx2_func, nullptr)
            << "Missing avx2_func!" << width_ << " " << height_;

        uint32_t ref_sad = reference_sad();
        uint32_t non_avx2_sad = non_avx2_func(source_data_,
                                              source_stride_,
                                              ref_1_data_,
                                              ref_1_stride_,
                                              width_,
                                              height_);
        uint32_t avx2_sad = avx2_func(source_data_,
                                      source_stride_,
                                      ref_1_data_,
                                      ref_1_stride_,
                                      width_,
                                      height_);

        ASSERT_EQ(ref_sad, non_avx2_sad) << "Case 1 error";
        ASSERT_EQ(ref_sad, avx2_sad) << "Case 2 error";
        ASSERT_EQ(non_avx2_sad, avx2_sad) << "Case 3 error";
    }
};

TEST_P(SADTestSubSample, SADTestSubSample) {
    check_sad();
}

INSTANTIATE_TEST_CASE_P(
    SAD, SADTestSubSample,
    ::testing::Combine(::testing::ValuesIn(vectors),
                       ::testing::ValuesIn(resolutions_all)));

class SADTest : public ::testing::WithParamInterface<SadTestParam>,
                public SADTestBase {
  public:
    SADTest()
        : SADTestBase(std::get<0>(TEST_GET_PARAM(1)),
                      std::get<1>(TEST_GET_PARAM(1)), TEST_GET_PARAM(0)) {
    }

  protected:
    void check_sad() {
        EB_SADKERNELNxM_TYPE non_avx2_func = nullptr;
        EB_SADKERNELNxM_TYPE avx2_func = nullptr;
        switch (width_) {
        case 64: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][8];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][8];
            break;
        }
        case 48: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][6];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][6];
            break;
        }
        case 32: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][4];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][4];
            break;
        }
        case 24: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][3];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][3];
            break;
        }
        case 16: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][2];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][2];
            break;
        }
        case 8: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][1];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][1];
            break;
        }
        case 4: {
            non_avx2_func = NxMSadKernel_funcPtrArray[ASM_NON_AVX2][0];
            avx2_func = NxMSadKernel_funcPtrArray[ASM_AVX2][0];
            break;
        }
        default: break;
        }
        ASSERT_NE(non_avx2_func, nullptr)
            << "Missing non_avx2_func!" << width_ << " " << height_;
        ASSERT_NE(avx2_func, nullptr)
            << "Missing avx2_func!" << width_ << " " << height_;

        uint32_t ref_sad = reference_sad();
        uint32_t non_avx2_sad = non_avx2_func(source_data_,
                                              source_stride_,
                                              ref_1_data_,
                                              ref_1_stride_,
                                              width_,
                                              height_);
        uint32_t avx2_sad = avx2_func(source_data_,
                                      source_stride_,
                                      ref_1_data_,
                                      ref_1_stride_,
                                      width_,
                                      height_);

        ASSERT_EQ(ref_sad, non_avx2_sad) << "Case 1 error";
        ASSERT_EQ(ref_sad, avx2_sad) << "Case 2 error";
        ASSERT_EQ(non_avx2_sad, avx2_sad) << "Case 3 error";
    }
};

TEST_P(SADTest, SADTest) {
    check_sad();
}

INSTANTIATE_TEST_CASE_P(
    SAD, SADTest,
    ::testing::Combine(::testing::ValuesIn(vectors),
                       ::testing::ValuesIn(resolutions_all)));

class SADAvgTest : public ::testing::WithParamInterface<SadTestParam>,
                   public SADTestBase {
  public:
    SADAvgTest()
        : SADTestBase(std::get<0>(TEST_GET_PARAM(1)),
                      std::get<1>(TEST_GET_PARAM(1)), TEST_GET_PARAM(0)) {
    }

  protected:
    void check_sad_avg() {
        EB_SADAVGKERNELNxM_TYPE non_avx2_func = nullptr;
        EB_SADAVGKERNELNxM_TYPE avx2_func = nullptr;
        switch (width_) {
        case 64: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][8];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][8];
            break;
        }
        case 48: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][6];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][6];
            break;
        }
        case 32: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][4];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][4];
            break;
        }
        case 24: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][3];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][3];
            break;
        }
        case 16: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][2];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][2];
            break;
        }
        case 8: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][1];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][1];
            break;
        }
        case 4: {
            non_avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_NON_AVX2][0];
            avx2_func = NxMSadAveragingKernel_funcPtrArray[ASM_AVX2][0];
            break;
        }
        default: break;
        }
        ASSERT_NE(non_avx2_func, nullptr)
            << "Missing non_avx2_func!" << width_ << " " << height_;
        ASSERT_NE(avx2_func, nullptr)
            << "Missing avx2_func!" << width_ << " " << height_;

        uint32_t ref_sad = reference_sad_avg();
        uint32_t non_avx2_sad = non_avx2_func(source_data_,
                                              source_stride_,
                                              ref_1_data_,
                                              ref_1_stride_,
                                              ref_2_data_,
                                              ref_2_stride_,
                                              width_,
                                              height_);
        uint32_t avx2_sad = avx2_func(source_data_,
                                      source_stride_,
                                      ref_1_data_,
                                      ref_1_stride_,
                                      ref_2_data_,
                                      ref_2_stride_,
                                      width_,
                                      height_);

        ASSERT_EQ(ref_sad, non_avx2_sad) << "Case 1 error";
        ASSERT_EQ(ref_sad, avx2_sad) << "Case 2 error";
        ASSERT_EQ(non_avx2_sad, avx2_sad) << "Case 3 error";
    }
};

TEST_P(SADAvgTest, SADAvgTest) {
    check_sad_avg();
}

INSTANTIATE_TEST_CASE_P(
    SAD, SADAvgTest,
    ::testing::Combine(::testing::ValuesIn(vectors),
                       ::testing::ValuesIn(resolutions_all)));

}  // namespace