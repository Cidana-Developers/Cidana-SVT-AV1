/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
#ifndef _E2E_TEST_VECTOR_
#define _E2E_TEST_VECTOR_

#include "VideoSource.h"

namespace svt_av1_e2e_test_vector {

typedef enum TestVideoVectorFormatType {
    YUM_VIDEO_FILE,
    Y4M_VIDEO_FILE
} TestVideoVectorFormatType;

typedef std::tuple<std::string, TestVideoVectorFormatType, VideoImageFormat,
                   uint32_t, uint32_t, uint8_t>
    TestVideoVector;

static const TestVideoVector video_src_vectors[] = {
    TestVideoVector{"../../test/vectors/park_joy_90p_8_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8},
    TestVideoVector{"../../test/vectors/park_joy_90p_8_422.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8},
    TestVideoVector{"../../test/vectors/park_joy_90p_8_444.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    8},
    TestVideoVector{"../../test/vectors/park_joy_90p_10_420.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    10},
    TestVideoVector{"../../test/vectors/park_joy_90p_10_422.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    10},
    TestVideoVector{"../../test/vectors/park_joy_90p_10_444.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    160,
                    90,
                    10},
    TestVideoVector{"../../test/vectors/park_joy_420_720p50.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    1280,
                    720,
                    8},
    TestVideoVector{"../../test/vectors/park_joy_422_720p50.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_422,
                    1280,
                    720,
                    8},
    TestVideoVector{"../../test/vectors/park_joy_444_720p50.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_444,
                    1280,
                    720,
                    8},
};

static const TestVideoVector recon_file_vectors[] = {
    TestVideoVector{"../../test/vectors/hantro_collage_w352h288.yuv",
                    YUM_VIDEO_FILE,
                    IMG_FMT_420,
                    352,
                    288,
                    8},
};

static const TestVideoVector smoking_vectors[] = {
    TestVideoVector{"../../test/vectors/screendata.y4m",
                    Y4M_VIDEO_FILE,
                    IMG_FMT_420,
                    640,
                    480,
                    8},
};
}  // namespace svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
