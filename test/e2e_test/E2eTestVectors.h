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

/** TestVideoVectorFormatType is enumerate type of input video file format */
typedef enum TestVideoVectorFormatType {
    YUM_VIDEO_FILE,
    Y4M_VIDEO_FILE
} TestVideoVectorFormatType;

/** TestVideoVector is tuple of test params in a test case */
typedef std::tuple<std::string, TestVideoVectorFormatType, VideoImageFormat,
                   uint32_t, uint32_t, uint8_t, bool>
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
    TestVideoVector{"park_joy_420_720p50.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    1280,
                    720,
                    8,
                    false},
};

static const TestVideoVector comformance_test_vectors[] = {
    TestVideoVector{"jellyfish-420-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    320,
                    180,
                    8,
                    false},
    TestVideoVector{"jellyfish-420p10-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420P10_PACKED,
                    320,
                    180,
                    10,
                    false},
};

static const TestVideoVector smoking_vectors[] = {
    TestVideoVector{"jellyfish-420-180p.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    320,
                    180,
                    8,
                    false},
};
}  // namespace svt_av1_e2e_test_vector
/** @} */  // end of svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
