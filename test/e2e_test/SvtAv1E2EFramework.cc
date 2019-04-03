#include "EbSvtAv1Enc.h"
#include "Y4mVideoSource.h"
#include "YuvVideosource.h"
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"

#define INPUT_SIZE_576p_TH 0x90000    // 0.58 Million
#define INPUT_SIZE_1080i_TH 0xB71B0   // 0.75 Million
#define INPUT_SIZE_1080p_TH 0x1AB3F0  // 1.75 Million
#define INPUT_SIZE_4K_TH 0x29F630     // 2.75 Million
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(resolution_size) \
    ((resolution_size) < (INPUT_SIZE_1080i_TH)           \
         ? 0x1E8480                                      \
         : (resolution_size) < (INPUT_SIZE_1080p_TH)     \
               ? 0x2DC6C0                                \
               : (resolution_size) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0)

using namespace svt_av1_test_e2e;

SvtAv1E2ETestBase::SvtAv1E2ETestBase()
    : video_src_(SvtAv1E2ETestBase::prepare_video_src(GetParam())) {
    memset(&ctxt_, 0, sizeof(ctxt_));
}

SvtAv1E2ETestBase::~SvtAv1E2ETestBase() {
    if (video_src_) {
        delete video_src_;
        video_src_ = nullptr;
    }
}

void SvtAv1E2ETestBase::SetUp() {
    EbErrorType return_error = EB_ErrorNone;

	// check for video source
    ASSERT_NE(video_src_, nullptr) << "video source create failed!";
    return_error = video_src_->open_source();
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "open_source return error:" << return_error;
    // Check input parameters
    uint32_t width = video_src_->get_width_with_padding();
    uint32_t height = video_src_->get_height_with_padding();
    uint32_t bit_depth = video_src_->get_bit_depth();
    ASSERT_GE(width, 0) << "Video vector width error.";
    ASSERT_GE(height, 0) << "Video vector height error.";
    ASSERT_TRUE(bit_depth == 10 || bit_depth == 8)
        << "Video vector bitDepth error.";

    //
    // Init handle
    //
    return_error = eb_init_handle(&ctxt_.enc_handle, &ctxt_, &ctxt_.enc_params);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_init_handle return error:" << return_error;
    ASSERT_NE(ctxt_.enc_handle, nullptr)
        << "eb_init_handle return null handle.";

    ctxt_.enc_params.source_width = width;
    ctxt_.enc_params.source_height = height;
    ctxt_.enc_params.encoder_bit_depth = bit_depth;
    ctxt_.enc_params.recon_enabled = 0;
    // TODO: set parameter here?

    //
    // Prepare buffer
    //
    // Input Buffer
    ctxt_.input_picture_buffer = new EbBufferHeaderType;
    ASSERT_NE(ctxt_.input_picture_buffer, nullptr)
        << "Malloc memory for inputPictureBuffer failed.";
    ctxt_.input_picture_buffer->p_buffer = nullptr;
    ctxt_.input_picture_buffer->size = sizeof(EbBufferHeaderType);
    ctxt_.input_picture_buffer->p_app_private = nullptr;
    ctxt_.input_picture_buffer->pic_type = EB_AV1_INVALID_PICTURE;

    // Output buffer
    ctxt_.output_stream_buffer = new EbBufferHeaderType;
    ASSERT_NE(ctxt_.output_stream_buffer, nullptr)
        << "Malloc memory for outputStreamBuffer failed.";
    ctxt_.output_stream_buffer->p_buffer =
        new uint8_t[EB_OUTPUTSTREAMBUFFERSIZE_MACRO(width * height)];
    ASSERT_NE(ctxt_.output_stream_buffer->p_buffer, nullptr)
        << "Malloc memory for outputStreamBuffer->p_buffer failed.";
    ctxt_.output_stream_buffer->size = sizeof(EbBufferHeaderType);
    ctxt_.output_stream_buffer->n_alloc_len =
        EB_OUTPUTSTREAMBUFFERSIZE_MACRO(width * height);
    ctxt_.output_stream_buffer->p_app_private = nullptr;
    ctxt_.output_stream_buffer->pic_type = EB_AV1_INVALID_PICTURE;
}

void SvtAv1E2ETestBase::TearDown() {
    EbErrorType return_error = EB_ErrorNone;

    // Destruct the component
    return_error = eb_deinit_handle(ctxt_.enc_handle);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_deinit_handle return error:" << return_error;
    ctxt_.enc_handle = nullptr;

    // Clear
    if (ctxt_.output_stream_buffer != nullptr) {
        if (ctxt_.output_stream_buffer->p_buffer != nullptr) {
            delete[] ctxt_.output_stream_buffer->p_buffer;
        }
        delete[] ctxt_.output_stream_buffer;
        ctxt_.output_stream_buffer = nullptr;
    }
}

void SvtAv1E2ETestBase::init_test() {
    EbErrorType return_error = EB_ErrorNone;
    return_error =
        eb_svt_enc_set_parameter(ctxt_.enc_handle, &ctxt_.enc_params);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_svt_enc_set_parameter return error:" << return_error;

    return_error = eb_init_encoder(ctxt_.enc_handle);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_init_encoder return error:" << return_error;

    // Get ivf header
    return_error =
        eb_svt_enc_stream_header(ctxt_.enc_handle, &ctxt_.output_stream_buffer);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_svt_enc_stream_header return error:" << return_error;
    ASSERT_NE(ctxt_.output_stream_buffer, nullptr)
        << "eb_svt_enc_stream_header return null output buffer."
        << return_error;
}

void SvtAv1E2ETestBase::close_test() {
    EbErrorType return_error = EB_ErrorNone;
    // Deinit
    return_error = eb_deinit_encoder(ctxt_.enc_handle);
    ASSERT_EQ(return_error, EB_ErrorNone)
        << "eb_deinit_encoder return error:" << return_error;
}

VideoSource *SvtAv1E2ETestBase::prepare_video_src(
    const TestVideoVector &vector) {
    VideoSource *video_src = nullptr;
    switch (std::get<1>(vector)) {
    case YUM_VIDEO_FILE:
        video_src = new YuvVideoSource(std::get<0>(vector),
                                       std::get<2>(vector),
                                       std::get<3>(vector),
                                       std::get<4>(vector),
                                       std::get<5>(vector));
        break;
    case Y4M_VIDEO_FILE:
        video_src = new Y4MVideoSource(std::get<0>(vector),
                                       std::get<2>(vector),
                                       std::get<3>(vector),
                                       std::get<4>(vector),
                                       std::get<5>(vector));
        break;
    default: assert(0); break;
    }
    return video_src;
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::run_encode_process() {
    EbErrorType return_error = EB_ErrorNone;
    uint8_t *frame = nullptr;
    do {
        frame = (uint8_t *)video_src_->get_next_frame();
        if (frame != nullptr) {
            // Fill in Buffers Header control data
            ctxt_.input_picture_buffer->p_buffer = frame;
            ctxt_.input_picture_buffer->n_filled_len =
                video_src_->get_frame_size();
            ctxt_.input_picture_buffer->flags = 0;
            ctxt_.input_picture_buffer->p_app_private = nullptr;
            ctxt_.input_picture_buffer->pts = video_src_->get_frame_index();
            ctxt_.input_picture_buffer->pic_type = EB_AV1_INVALID_PICTURE;
            // Send the picture
            EXPECT_EQ(EB_ErrorNone,
                      eb_svt_enc_send_picture(ctxt_.enc_handle,
                                              ctxt_.input_picture_buffer))
                << "eb_svt_enc_send_picture error at: "
                << ctxt_.input_picture_buffer->pts;
        } else {
            EbBufferHeaderType headerPtrLast;
            headerPtrLast.n_alloc_len = 0;
            headerPtrLast.n_filled_len = 0;
            headerPtrLast.n_tick_count = 0;
            headerPtrLast.p_app_private = nullptr;
            headerPtrLast.flags = EB_BUFFERFLAG_EOS;
            headerPtrLast.p_buffer = nullptr;
            ctxt_.input_picture_buffer->flags = EB_BUFFERFLAG_EOS;
            EXPECT_EQ(EB_ErrorNone,
                      eb_svt_enc_send_picture(ctxt_.enc_handle, &headerPtrLast))
                << "eb_svt_enc_send_picture EOS error";
        }

        // recon
        if (frame && recon_sink_) {
            get_recon_frame();
        }

        // non-blocking call
        EbBufferHeaderType *headerPtr = nullptr;
        return_error = eb_svt_get_packet(
            ctxt_.enc_handle, &headerPtr, (frame == nullptr ? 1 : 0));
        ASSERT_NE(return_error, EB_ErrorMax)
            << "Error while encoding, code:" << headerPtr->flags;
        // Release the output buffer
        if (headerPtr != nullptr) {
            eb_svt_release_out_buffer(&headerPtr);
        }
        // TODO: plug-in decoder to verify the compressed data
    } while ((frame != nullptr));
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::get_recon_frame() {
    do {
        ReconSink::ReconMug *new_mug = recon_sink_->get_empty_mug();
        ASSERT_NE(new_mug, nullptr) << "can not get new mug for recon frame!!";
        ASSERT_NE(new_mug->mug_buf, nullptr)
            << "can not get new mug for recon frame!!";

        EbBufferHeaderType recon_frame = {0};
        recon_frame.size = sizeof(EbBufferHeaderType);
        recon_frame.p_buffer = new_mug->mug_buf;
        recon_frame.n_alloc_len = new_mug->mug_size;
        recon_frame.p_app_private = nullptr;
        // non-blocking call until all input frames are sent
        EbErrorType recon_status =
            eb_svt_get_recon(ctxt_.enc_handle, &recon_frame);
        ASSERT_NE(recon_status, EB_ErrorMax)
            << "Error while outputing recon, code:" << recon_frame.flags;
        if (recon_status == EB_NoErrorEmptyQueue) {
            recon_sink_->pour_mug(new_mug);
            break;
        } else {
            new_mug->filled_size = recon_frame.n_filled_len;
            new_mug->time_stamp = recon_frame.pts;
            new_mug->tag = recon_frame.flags;
            printf("recon image frame: %d\n", new_mug->time_stamp);
            recon_sink_->fill_mug(new_mug);
        }
    } while (true);
}
