#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"

namespace svt_av1_test {

typedef struct {
    EbComponentType* enc_handle;
    EbSvtAv1EncConfiguration enc_params;
} SvtAv1Context;

}  // namespace svt_av1_test
