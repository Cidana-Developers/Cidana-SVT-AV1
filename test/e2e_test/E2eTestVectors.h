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
    {"../../test/vectors/hantro_collage_w352h288.yuv",
     YUM_VIDEO_FILE,
     IMG_FMT_420,
     352,
     288,
     8},
    {"../../test/vectors/screendata.y4m",
     Y4M_VIDEO_FILE,
     IMG_FMT_420,
     640,
     480,
     8},
};

}  // namespace svt_av1_e2e_test_vector

#endif  // _E2E_TEST_VECTOR_
