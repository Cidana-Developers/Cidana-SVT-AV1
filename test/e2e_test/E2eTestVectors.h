/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file E2eTestVectors.h
 *
 * @brief Defines test vectors for End to End test
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#ifndef _E2E_TEST_VECTOR_
#define _E2E_TEST_VECTOR_

#include "VideoSource.h"

/** @defgroup svt_av1_e2e_test_vector Test vectors for E2E test
 *  Defines the test vectors of E2E test, with file-type, width, height and
 * file-path
 *  @{
 */
namespace svt_av1_e2e_test_vector {

/** TestVectorFormat is enumerate type of input video file format */
typedef enum TestVectorFormat {
    YUV_VIDEO_FILE,
    Y4M_VIDEO_FILE,
    DUMMY_SOURCE
} TestVectorFormat;

/** TestVideoVector is tuple of test params in a test case */
typedef std::tuple<std::string,      /**< file name */
                   TestVectorFormat, /**< file format */
                   VideoColorFormat, /**< color format */
                   uint32_t,         /**< width */
                   uint32_t,         /**< height */
                   uint8_t,          /**< bit depth */
                   bool,             /**< compressed 2-bit in 10-bit frame */
                   uint32_t,         /**< start read position in frame */
                   uint32_t> /**< frames to test, (0) means full-frames*/
    TestVideoVector;

static const TestVideoVector video_src_vectors[] = {
    TestVideoVector{"park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{"park_joy_90p_10_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420P10_PACKED,
                    160,
                    90,
                    10,
                    false,
                    0,
                    0},
};

static const TestVideoVector comformance_test_vectors[] = {
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{"niklas_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{
        "color_bar", DUMMY_SOURCE, IMG_FMT_420, 640, 480, 8, false, 0, 0},
};

static const TestVideoVector smoking_vectors[] = {
    TestVideoVector{"park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8,
                    false,
                    0,
                    0},
};

static const TestVideoVector partial_src_frame_vectors[] = {
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8,
                    false,
                    100,
                    100},
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8,
                    false,
                    100,
                    0},
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8,
                    false,
                    0,
                    200},
};

static const TestVideoVector longtime_comformance_test_vectors[] = {
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    1920,
                    1080,
                    8,
                    false,
                    0,
                    3000},
};

static const TestVideoVector profile_vectors[] = {
    TestVideoVector{"jellyfish-420-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    320,
                    180,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{"jellyfish-422-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_422,
                    320,
                    180,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{"jellyfish-444-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_444,
                    320,
                    180,
                    8,
                    false,
                    0,
                    0},
};

static const TestVideoVector qp_death_test_vectors[] = {
    TestVideoVector{"Fruits_oranges,_jardin_japonais_2.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    1296,
                    864,
                    8,
                    false,
                    0,
                    0},
};

static const TestVideoVector src_resolution_death_test_vectors[] = {
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    952,
                    512,
                    8,
                    false,
                    0,
                    100},
    TestVideoVector{"kirland_640_480_30.yuv",
                    YUV_VIDEO_FILE,
                    IMG_FMT_420,
                    952,
                    704,
                    8,
                    false,
                    0,
                    100},
};

/** MultiInstVector */
typedef std::tuple<TestVideoVector, /**< video source */
                   uint32_t>        /**< instance number for test */
    MultiInstVector;

static const MultiInstVector multi_inst_vectors[] = {
    MultiInstVector{TestVideoVector{"kirland_640_480_30.yuv",
                                    YUV_VIDEO_FILE,
                                    IMG_FMT_420,
                                    640,
                                    480,
                                    8,
                                    false,
                                    0,
                                    0},

                    2},
};

}  // namespace svt_av1_e2e_test_vector
/** @} */  // end of svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
