/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file SvtAv1E2EMultiInstantTest.cc
 *
 * @brief Impelmentation of multipule instant of encoder test at same time
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"

using namespace svt_av1_e2e_test;

class EncTestCtxt : public ::testing::Test {
  public:
    EncTestCtxt() = delete;
    EncTestCtxt(const TestVideoVector& vector, const uint32_t id) {
        video_src_ = SvtAv1E2ETestBase::prepare_video_src(vector);
        memset(&ctxt_, 0, sizeof(ctxt_));
        recon_sink_ = nullptr;
        refer_dec_ = nullptr;
        obu_frame_header_size_ = 0;
        collect_ = nullptr;
        ref_compare_ = nullptr;
        channel_id_ = id;
    }
    ~EncTestCtxt() {
        if (video_src_) {
            delete video_src_;
            video_src_ = nullptr;
        }
        if (recon_sink_) {
            delete recon_sink_;
            recon_sink_ = nullptr;
        }
        if (refer_dec_) {
            delete refer_dec_;
            refer_dec_ = nullptr;
        }
        if (collect_) {
            delete collect_;
            collect_ = nullptr;
        }
        if (ref_compare_) {
            delete ref_compare_;
            ref_compare_ = nullptr;
        }
    }

  public:
    void SetUp() override {
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
        return_error =
            eb_init_handle(&ctxt_.enc_handle, &ctxt_, &ctxt_.enc_params);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_init_handle return error:" << return_error;
        ASSERT_NE(ctxt_.enc_handle, nullptr)
            << "eb_init_handle return null handle.";
        ctxt_.enc_params.source_width = width;
        ctxt_.enc_params.source_height = height;
        ctxt_.enc_params.encoder_bit_depth = bit_depth;
        ctxt_.enc_params.compressed_ten_bit_format =
            video_src_->get_compressed_10bit_mode();
        /** setup channel id for instant identify */
        ctxt_.enc_params.channel_id = channel_id_;
        // trans_src_param();
        return_error =
            eb_svt_enc_set_parameter(ctxt_.enc_handle, &ctxt_.enc_params);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_svt_enc_set_parameter return error:" << return_error;

        return_error = eb_init_encoder(ctxt_.enc_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_init_encoder return error:" << return_error;

#if TILES
        EbBool has_tiles = (EbBool)(ctxt_.enc_params.tile_columns ||
                                    ctxt_.enc_params.tile_rows);
#else
        EbBool has_tiles = (EbBool)EB_FALSE;
#endif
        obu_frame_header_size_ =
            has_tiles ? OBU_FRAME_HEADER_SIZE + 1 : OBU_FRAME_HEADER_SIZE;

        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_sink_ = create_recon_sink(param);
        ASSERT_NE(recon_sink_, nullptr) << "can not create recon sink!!";
        ctxt_.enc_params.recon_enabled = 1;

        // create reference decoder
        refer_dec_ = create_reference_decoder();
        ASSERT_NE(refer_dec_, nullptr) << "can not create reference decoder!!";

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

    void TearDown() override {
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

        ASSERT_NE(video_src_, nullptr);
        video_src_->close_source();
    }

  protected:
    uint32_t channel_id_;           /**< channel id for encoder instance*/
    VideoSource* video_src_;        /**< video source context */
    SvtAv1Context ctxt_;            /**< AV1 encoder context */
    ReconSink* recon_sink_;         /**< reconstruction frame collection */
    RefDecoder* refer_dec_;         /**< reference decoder context */
    uint8_t obu_frame_header_size_; /**< size of obu frame header */
    PerformanceCollect* collect_;   /**< performance and time collection*/
    ICompareSink* ref_compare_; /**< sink of reference to compare with recon*/
};
