#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"

using namespace svt_av1_test_e2e;
using namespace svt_av1_e2e_test_vector;


class SvtAv1E2ETest : public SvtAv1E2ETestBase {
  protected:
    SvtAv1E2ETest(){};
    virtual ~SvtAv1E2ETest(){};

  public:
    virtual void run_encode_process() final override {
        EbErrorType return_error = EB_ErrorNone;
        int32_t frame_count = 0;
        uint8_t *frame = nullptr;
        do {
            frame = (uint8_t *)video_src_->get_next_frame();

            if (frame != nullptr) {
                printf("DISP: %d, size:%d\n",
                       frame_count,
                       video_src_->get_frame_size());
                // Fill in Buffers Header control data
                ctxt_.input_picture_buffer->p_buffer = frame;
                ctxt_.input_picture_buffer->n_filled_len =
                    video_src_->get_frame_size();
                ctxt_.input_picture_buffer->flags = 0;
                ctxt_.input_picture_buffer->p_app_private = nullptr;
                ctxt_.input_picture_buffer->pts = frame_count++;
                ctxt_.input_picture_buffer->pic_type = EB_AV1_INVALID_PICTURE;
                // Send the picture
                eb_svt_enc_send_picture(ctxt_.enc_handle,
                                        ctxt_.input_picture_buffer);
            } else {
                EbBufferHeaderType headerPtrLast;
                headerPtrLast.n_alloc_len = 0;
                headerPtrLast.n_filled_len = 0;
                headerPtrLast.n_tick_count = 0;
                headerPtrLast.p_app_private = nullptr;
                headerPtrLast.flags = EB_BUFFERFLAG_EOS;
                headerPtrLast.p_buffer = nullptr;
                ctxt_.input_picture_buffer->flags = EB_BUFFERFLAG_EOS;

                eb_svt_enc_send_picture(ctxt_.enc_handle, &headerPtrLast);
            }

            // non-blocking call
            EbBufferHeaderType *headerPtr = nullptr;
            return_error = eb_svt_get_packet(
                ctxt_.enc_handle, &headerPtr, (frame == nullptr ? 1 : 0));
            ASSERT_NE(return_error, EB_ErrorMax)
                << "nError while encoding, code:" << headerPtr->flags;
            // Release the output buffer
            if (headerPtr != nullptr) {
                eb_svt_release_out_buffer(&headerPtr);
            }
        } while ((frame != nullptr));
    }
};

TEST_P(SvtAv1E2ETest, run_conformance_test) {
    init_test();
    run_encode_process();
    close_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ETest,
                        ::testing::ValuesIn(video_src_vectors));
