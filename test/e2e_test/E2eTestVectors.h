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
    Y4M_VIDEO_FILE
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
                   uint32_t> /**< frames to test, (-1) means full-frames*/
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
                    -1},
    TestVideoVector{"park_joy_420_720p50.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    1280,
                    720,
                    8,
                    false,
                    0,
                    0},
};

static const TestVideoVector comformance_test_vectors[] = {
    TestVideoVector{"jellyfish-420-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    320,
                    180,
                    8,
                    false,
                    0,
                    0},
    TestVideoVector{"jellyfish-420p10-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420P10_PACKED,
                    320,
                    180,
                    10,
                    false,
                    0,
                    0},
};

static const TestVideoVector smoking_vectors[] = {
    TestVideoVector{"jellyfish-420-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    320,
                    180,
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
                    0,
                    0},
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

}  // namespace svt_av1_e2e_test_vector
/** @} */  // end of svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
