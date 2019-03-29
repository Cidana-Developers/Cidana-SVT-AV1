#ifndef _SVT_AV1_E2E_FRAMEWORK_H_
#define _SVT_AV1_E2E_FRAMEWORK_H_

#include "E2eTestVectors.h"

namespace svt_av1_test_e2e {

using namespace svt_av1_e2e_test_vector;

typedef struct {
    EbComponentType *enc_handle;
    EbSvtAv1EncConfiguration enc_params;
    EbBufferHeaderType *output_stream_buffer;
    EbBufferHeaderType *input_picture_buffer;
} SvtAv1Context;

class SvtAv1E2ETestBase : public ::testing::TestWithParam<TestVideoVector> {
  public:
    SvtAv1E2ETestBase();
    virtual ~SvtAv1E2ETestBase();

  protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
    virtual void init_test();
    virtual void close_test();
    virtual void run_encode_process() = 0;

  private:
    static VideoSource *prepare_video_src(const TestVideoVector &vector);

  protected:
    VideoSource *video_src_;
    SvtAv1Context ctxt_;
};

}  // namespace svt_av1_test_e2e

#endif  //_SVT_AV1_E2E_FRAMEWORK_H_
