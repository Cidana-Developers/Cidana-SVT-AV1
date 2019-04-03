#ifndef _SVT_AV1_E2E_FRAMEWORK_H_
#define _SVT_AV1_E2E_FRAMEWORK_H_

#include "E2eTestVectors.h"
#include "ReconSink.h"
#include "VideoMonitor.h"

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

class SvtAv1E2ETestFramework : public SvtAv1E2ETestBase {
  protected:
    SvtAv1E2ETestFramework() {
        recon_sink_ = nullptr;
        monitor_ = nullptr;
    }
    virtual ~SvtAv1E2ETestFramework() {
        if (recon_sink_) {
            delete recon_sink_;
            recon_sink_ = nullptr;
        }
        if (monitor_) {
            delete monitor_;
            monitor_ = nullptr;
        }
    }

  protected:
    virtual void run_encode_process() final override;

  protected:
    // plug-in for test data
    // recon-data pin
    virtual void get_recon_frame();
    // decoder pin
    // psnr pin
  protected:
    ReconSink *recon_sink_;
    VideoMonitor *monitor_;
};

}  // namespace svt_av1_test_e2e

#endif  //_SVT_AV1_E2E_FRAMEWORK_H_
