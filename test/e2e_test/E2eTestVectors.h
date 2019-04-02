#ifndef _E2E_TEST_VECTOR_
#define _E2E_TEST_VECTOR_

#include "VideoSource.h"

namespace svt_av1_e2e_test_vector {

typedef enum TestVideoVectorFormatType {
    YUM_VIDEO_FILE,
    Y4M_VIDEO_FILE
} TestVideoVectorFormatType;

typedef struct TestVideoVector {
    char *file_name;
    TestVideoVectorFormatType file_format;
    VideoImageFormat img_format;
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
} TestVideoVector;

static const TestVideoVector video_src_vectors[] = {
    //     {"../../test/vectors/hantro_collage_w352h288.yuv",
    //      YUM_VIDEO_FILE,
    //      IMG_FMT_420,
    //      352,
    //      288,
    //      8},
    {"../../test/vectors/park_joy_90p_8_420.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     8},
    {"../../test/vectors/park_joy_90p_8_422.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     8},
    {"../../test/vectors/park_joy_90p_8_444.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     8},
    {"../../test/vectors/park_joy_90p_10_420.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     10},
    {"../../test/vectors/park_joy_90p_10_422.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     10},
    {"../../test/vectors/park_joy_90p_10_444.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     160,
     90,
     10},
    {"../../test/vectors/park_joy_420_720p50.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     1280,
     720,
     8},
    {"../../test/vectors/park_joy_422_720p50.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_422,
     1280,
     720,
     8},
    {"../../test/vectors/park_joy_444_720p50.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_444,
     1280,
     720,
     8},
};

}  // namespace svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
