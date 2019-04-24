/*
 * Copyright(c) 2019 Intel Corporation
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
typedef std::tuple<std::string, TestVectorFormat, VideoColorFormat, uint32_t,
                   uint32_t, uint8_t, bool>
    TestVideoVector;

static const TestVideoVector video_src_vectors[] = {
    TestVideoVector{"park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8,
                    false},
    TestVideoVector{"park_joy_90p_10_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420P10_PACKED,
                    160,
                    90,
                    10,
                    false},
};

static const TestVideoVector comformance_test_vectors[] = {
    TestVideoVector{"park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8,
                    false},
    TestVideoVector{"park_joy_90p_10_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420P10_PACKED,
                    160,
                    90,
                    10,
                    false},
};

static const TestVideoVector smoking_vectors[] = {
    TestVideoVector{"park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8,
                    false},
};
}  // namespace svt_av1_e2e_test_vector
/** @} */  // end of svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
