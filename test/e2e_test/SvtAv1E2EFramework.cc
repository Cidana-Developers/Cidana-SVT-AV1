/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file SvtAv1E2EFramework.cc
 *
 * @brief Impelmentation of End to End test framework
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "EbSvtAv1Enc.h"
#include "Y4mVideoSource.h"
#include "YuvVideosource.h"
#include "gtest/gtest.h"
#include "RefDecoder.h"
#include "SvtAv1E2EFramework.h"
#include "math.h"

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

#if _WIN32
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#define FOPEN(f, s, m) fopen_s(&f, s, m)
#else
#define fseeko64 fseek
#define ftello64 ftell
#define FOPEN(f, s, m) f = fopen(s, m)
#endif

#define LONG_ENCODE_FRAME_ENCODE 4000
#define SPEED_MEASUREMENT_INTERVAL 2000
#define START_STEADY_STATE 1000
#define AV1_FOURCC 0x31305641  // used for ivf header
#define IVF_STREAM_HEADER_SIZE 32
#define IVF_FRAME_HEADER_SIZE 12
#define OBU_FRAME_HEADER_SIZE 3
#define TD_SIZE 2
static __inline void mem_put_le32(void *vmem, int32_t val) {
    uint8_t *mem = (uint8_t *)vmem;
    mem[0] = (uint8_t)((val >> 0) & 0xff);
    mem[1] = (uint8_t)((val >> 8) & 0xff);
    mem[2] = (uint8_t)((val >> 16) & 0xff);
    mem[3] = (uint8_t)((val >> 24) & 0xff);
}
#define MEM_VALUE_T_SZ_BITS (sizeof(MEM_VALUE_T) << 3)
static __inline void mem_put_le16(void *vmem, int32_t val) {
    uint8_t *mem = (uint8_t *)vmem;
    mem[0] = (uint8_t)((val >> 0) & 0xff);
    mem[1] = (uint8_t)((val >> 8) & 0xff);
}

using namespace svt_av1_test_e2e;

bool compare_image(const ReconSink::ReconMug *recon, VideoFrame *ref_frame) {
    const uint32_t width = ref_frame->disp_width;
    const uint32_t height = ref_frame->disp_height;
    unsigned int i = 0;
    if (ref_frame->bits_per_sample == 8) {
        for (int l = 0; l < height; l++) {
            const uint8_t *s = recon->mug_buf + l * width;
            const uint8_t *d = ref_frame->planes[0] + l * ref_frame->stride[0];
            for (int r = 0; r < width; r++) {
                if (s[r] != d[r * 2])  // ref decoder use 2bytes to store 8 bits
                                       // depth pix.
                    return false;
                i++;
            }
        }

        for (int l = 0; l < (height >> 1); l++) {
            const uint8_t *s =
                (recon->mug_buf + width * height) + l * (width >> 1);
            const uint8_t *d = ref_frame->planes[1] + l * ref_frame->stride[1];
            for (int r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r * 2])
                    return false;
                i++;
            }
        }

        for (int l = 0; l < (height >> 1); l++) {
            const uint8_t *s =
                (recon->mug_buf + width * height * 5 / 4) + l * (width >> 1);
            const uint8_t *d = ref_frame->planes[2] + l * ref_frame->stride[2];
            for (int r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r * 2])
                    return false;
                i++;
            }
        }
    } else  // 10bit mode.
    {
        for (int l = 0; l < height; l++) {
            const uint16_t *s = (uint16_t *)(recon->mug_buf + l * width * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[0] + l * ref_frame->stride[0]);
            for (int r = 0; r < width; r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }

        for (int l = 0; l < (height >> 1); l++) {
            const uint16_t *s =
                (uint16_t *)(recon->mug_buf + width * height * 2 +
                             l * (width >> 1) * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[1] + l * ref_frame->stride[1]);
            for (int r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }

        for (int l = 0; l < (height >> 1); l++) {
            const uint16_t *s =
                (uint16_t *)(recon->mug_buf + width * height * 5 / 4 * 2 +
                             l * (width >> 1) * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[2] + l * ref_frame->stride[2]);
            for (int r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }
    }
    return true;
}

double psnr_8bit(const uint8_t *p1, const uint8_t *p2, const uint32_t size) {
    double mse = 0.0;
    for (int i = 0; i < size; i++) {
        const uint8_t I = p1[i];
        const uint8_t K = p2[i];
        const int32_t diff = I - K;
        mse += diff * diff;
    }
    mse /= size;

    double psnr = INFINITY;

    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((255 * 255) / mse);
    }
    return psnr;
}

float psnr_10bit(const uint16_t *p1, const uint16_t *p2, const uint32_t size) {
    double mse = 0.0;
    for (int i = 0; i < size; i++) {
        const uint16_t I = p1[i];
        const uint16_t K = p2[i];
        const int32_t diff = I - K;
        mse += diff * diff;
    }
    mse /= size;

    double psnr = INFINITY;

    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((1023 * 1023) / mse);
    }
    return psnr;
}

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
    ctxt_.enc_params.ten_bit_format = 1;
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

    ASSERT_NE(video_src_, nullptr);
    video_src_->close_source();
}

void SvtAv1E2ETestBase::init_test() {
    EbErrorType return_error = EB_ErrorNone;
    ctxt_.enc_params.frames_to_be_encoded = video_src_->get_frame_count();
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

void svt_av1_test_e2e::SvtAv1E2ETestFramework::init_test() {
    SvtAv1E2ETestBase::init_test();
#if TILES
    EbBool has_tiles =
        (EbBool)(ctxt_.enc_params.tile_columns || ctxt_.enc_params.tile_rows);
#else
    EbBool has_tiles = (EbBool)EB_FALSE;
#endif
    obu_frame_header_size_ =
        has_tiles ? OBU_FRAME_HEADER_SIZE + 1 : OBU_FRAME_HEADER_SIZE;
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::run_encode_process() {
    EbErrorType return_error = EB_ErrorNone;

    uint32_t frame_count = video_src_->get_frame_count();
    ASSERT_GT(frame_count, 0) << "video srouce file does not contain frame!!";
    if (recon_sink_) {
        recon_sink_->set_frame_count(frame_count);
    }

    if (output_file_) {
        write_output_header();
    }

    uint8_t *frame = nullptr;
    bool src_file_eos = false;
    bool enc_file_eos = false;
    bool rec_file_eos = recon_sink_ ? false : true;
    int ref_frame_count = 0;
    do {
        if (!src_file_eos) {
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
                          return_error = eb_svt_enc_send_picture(
                              ctxt_.enc_handle, ctxt_.input_picture_buffer))
                    << "eb_svt_enc_send_picture error at: "
                    << ctxt_.input_picture_buffer->pts;
            } else if (!src_file_eos) {
                src_file_eos = true;
                EbBufferHeaderType headerPtrLast;
                headerPtrLast.n_alloc_len = 0;
                headerPtrLast.n_filled_len = 0;
                headerPtrLast.n_tick_count = 0;
                headerPtrLast.p_app_private = nullptr;
                headerPtrLast.flags = EB_BUFFERFLAG_EOS;
                headerPtrLast.p_buffer = nullptr;
                ctxt_.input_picture_buffer->flags = EB_BUFFERFLAG_EOS;
                EXPECT_EQ(EB_ErrorNone,
                          return_error = eb_svt_enc_send_picture(
                              ctxt_.enc_handle, &headerPtrLast))
                    << "eb_svt_enc_send_picture EOS error";
            }
        }

        // recon
        if (recon_sink_ && !rec_file_eos) {
            get_recon_frame();
            rec_file_eos = recon_sink_->is_compelete();
        }

        if (!enc_file_eos) {
            do {
                // non-blocking call
                EbBufferHeaderType *enc_out = nullptr;
                return_error = eb_svt_get_packet(
                    ctxt_.enc_handle, &enc_out, (frame == nullptr ? 1 : 0));
                ASSERT_NE(return_error, EB_ErrorMax)
                    << "Error while encoding, code:" << enc_out->flags;

                // process the output buffer
                if (return_error != EB_NoErrorEmptyQueue && enc_out) {
                    printf(
                        "Encoder "
                        "Order:\tdts:\t%ld\tpts:\t%ld\tSliceType:\t%d\n",
                        (long int)enc_out->dts,
                        (long int)enc_out->pts,
                        (int)enc_out->pic_type);
                    process_compress_data(enc_out);

                    if (enc_out->flags & EB_BUFFERFLAG_EOS) {
                        enc_file_eos = true;
                        printf("Encoder EOS\n");
                        break;
                    }
                } else
                    break;

                // Release the output buffer
                if (enc_out != nullptr) {
                    eb_svt_release_out_buffer(&enc_out);
                    // EXPECT_EQ(enc_out, nullptr)
                    //    << "enc_out buffer is not well released";
                }
            } while (src_file_eos);
        }
    } while (!rec_file_eos || !src_file_eos || !enc_file_eos);
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::write_output_header() {
    char header[IVF_STREAM_HEADER_SIZE];
    header[0] = 'D';
    header[1] = 'K';
    header[2] = 'I';
    header[3] = 'F';
    mem_put_le16(header + 4, 0);                                // version
    mem_put_le16(header + 6, 32);                               // header size
    mem_put_le32(header + 8, AV1_FOURCC);                       // fourcc
    mem_put_le16(header + 12, ctxt_.enc_params.source_width);   // width
    mem_put_le16(header + 14, ctxt_.enc_params.source_height);  // height
    if (ctxt_.enc_params.frame_rate_denominator != 0 &&
        ctxt_.enc_params.frame_rate_numerator != 0) {
        mem_put_le32(header + 16,
                     ctxt_.enc_params.frame_rate_numerator);  // rate
        mem_put_le32(header + 20,
                     ctxt_.enc_params.frame_rate_denominator);  // scale
    } else {
        mem_put_le32(header + 16,
                     (ctxt_.enc_params.frame_rate >> 16) * 1000);  // rate
        mem_put_le32(header + 20, 1000);                           // scale
    }
    mem_put_le32(header + 24, 0);  // length
    mem_put_le32(header + 28, 0);  // unused
    if (output_file_ && output_file_->file)
        fwrite(header, 1, IVF_STREAM_HEADER_SIZE, output_file_->file);
}

static void update_prev_ivf_header(
    svt_av1_test_e2e::SvtAv1E2ETestFramework::IvfFile *ivf) {
    char header[4];  // only for the number of bytes
    if (ivf && ivf->file && ivf->byte_count_since_ivf != 0) {
        fseeko64(
            ivf->file,
            (-(int32_t)(ivf->byte_count_since_ivf + IVF_FRAME_HEADER_SIZE)),
            SEEK_CUR);
        mem_put_le32(&header[0], (int32_t)(ivf->byte_count_since_ivf));
        fwrite(header, 1, 4, ivf->file);
        fseeko64(ivf->file,
                 (ivf->byte_count_since_ivf + IVF_FRAME_HEADER_SIZE - 4),
                 SEEK_CUR);
        ivf->byte_count_since_ivf = 0;
    }
}

static void write_ivf_frame_header(
    svt_av1_test_e2e::SvtAv1E2ETestFramework::IvfFile *ivf,
    uint32_t byte_count) {
    char header[IVF_FRAME_HEADER_SIZE];
    int32_t write_location = 0;

    mem_put_le32(&header[write_location], (int32_t)byte_count);
    write_location = write_location + 4;
    mem_put_le32(&header[write_location],
                 (int32_t)((ivf->ivf_count) & 0xFFFFFFFF));
    write_location = write_location + 4;
    mem_put_le32(&header[write_location], (int32_t)((ivf->ivf_count) >> 32));
    write_location = write_location + 4;

    ivf->byte_count_since_ivf = (byte_count);

    ivf->ivf_count++;
    fflush(stdout);

    if (ivf->file)
        fwrite(header, 1, IVF_FRAME_HEADER_SIZE, ivf->file);
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::write_compress_data(
    const EbBufferHeaderType *output) {
    switch (output->flags &
            0x00000006) {  // Check for the flags EB_BUFFERFLAG_HAS_TD and
                           // EB_BUFFERFLAG_SHOW_EXT

    case (EB_BUFFERFLAG_HAS_TD | EB_BUFFERFLAG_SHOW_EXT):

        // terminate previous ivf packet, update the combined size of packets
        // sent
        update_prev_ivf_header(output_file_);

        // Write a new IVF frame header to file as a TD is in the packet
        write_ivf_frame_header(
            output_file_,
            output->n_filled_len - (obu_frame_header_size_ + TD_SIZE));
        fwrite(output->p_buffer,
               1,
               output->n_filled_len - (obu_frame_header_size_ + TD_SIZE),
               output_file_->file);

        // An EB_BUFFERFLAG_SHOW_EXT means that another TD has been added to the
        // packet to show another frame, a new IVF is needed
        write_ivf_frame_header(output_file_,
                               (obu_frame_header_size_ + TD_SIZE));
        fwrite(output->p_buffer + output->n_filled_len -
                   (obu_frame_header_size_ + TD_SIZE),
               1,
               (obu_frame_header_size_ + TD_SIZE),
               output_file_->file);

        break;

    case (EB_BUFFERFLAG_HAS_TD):

        // terminate previous ivf packet, update the combined size of packets
        // sent
        update_prev_ivf_header(output_file_);

        // Write a new IVF frame header to file as a TD is in the packet
        write_ivf_frame_header(output_file_, output->n_filled_len);
        fwrite(output->p_buffer, 1, output->n_filled_len, output_file_->file);

        break;

    case (EB_BUFFERFLAG_SHOW_EXT):

        // this case means that there's only one TD in this packet and is
        // relater
        fwrite(output->p_buffer,
               1,
               output->n_filled_len - (obu_frame_header_size_ + TD_SIZE),
               output_file_->file);
        // this packet will be part of the previous IVF header
        output_file_->byte_count_since_ivf +=
            (output->n_filled_len - (obu_frame_header_size_ + TD_SIZE));

        // terminate previous ivf packet, update the combined size of packets
        // sent
        update_prev_ivf_header(output_file_);

        // An EB_BUFFERFLAG_SHOW_EXT means that another TD has been added to the
        // packet to show another frame, a new IVF is needed
        write_ivf_frame_header(output_file_,
                               (obu_frame_header_size_ + TD_SIZE));
        fwrite(output->p_buffer + output->n_filled_len -
                   (obu_frame_header_size_ + TD_SIZE),
               1,
               (obu_frame_header_size_ + TD_SIZE),
               output_file_->file);

        break;

    default:

        // This is a packet without a TD, write it straight to file
        fwrite(output->p_buffer, 1, output->n_filled_len, output_file_->file);

        // this packet will be part of the previous IVF header
        output_file_->byte_count_since_ivf += (output->n_filled_len);
        break;
    }
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::process_compress_data(
    const EbBufferHeaderType *data) {
    ASSERT_NE(data, nullptr);
    if (refer_dec_ == nullptr) {
        if (output_file_) {
            write_compress_data(data);
        }
        return;
    }

    if (data->flags & EB_BUFFERFLAG_SHOW_EXT) {
        uint32_t first_part_size =
            data->n_filled_len - obu_frame_header_size_ - TD_SIZE;
        decode_compress_data(data->p_buffer, first_part_size);
        decode_compress_data(data->p_buffer + first_part_size,
                             obu_frame_header_size_ + TD_SIZE);
    } else {
        decode_compress_data(data->p_buffer, data->n_filled_len);
    }
}

void svt_av1_test_e2e::SvtAv1E2ETestFramework::decode_compress_data(
    const uint8_t *data, uint32_t size) {
    ASSERT_NE(data, nullptr);
    ASSERT_GT(size, 0);

    // input the compressed data into decoder
    ASSERT_EQ(refer_dec_->process_data(data, size), RefDecoder::REF_CODEC_OK);

    VideoFrame ref_frame;
    memset(&ref_frame, 0, sizeof(ref_frame));
    while (refer_dec_->get_frame(ref_frame) == RefDecoder::REF_CODEC_OK) {
#ifdef ENABLE_DEBUG_MONITOR
        if (ref_monitor_ == nullptr) {
            ref_monitor_ = new VideoMonitor(ref_frame.width,
                                            ref_frame.height,
                                            ref_frame.stride[0],
                                            ref_frame.bits_per_sample,
                                            true,
                                            "Ref decode");
        }
        if (ref_monitor_) {
            ref_monitor_->draw_frame(
                ref_frame.planes[0], ref_frame.planes[1], ref_frame.planes[2]);
        }
#endif
        // compare tools
        if (recon_sink_) {
            // Compare ref decode output with recon
            // output.
            const ReconSink::ReconMug *mug =
                recon_sink_->take_mug(ref_frame.timestamp);
            if (mug) {
                uint32_t luma_len = video_src_->get_width_with_padding() *
                                    video_src_->get_height_with_padding() *
                                    (video_src_->get_bit_depth() > 8 ? 2 : 1);

#ifdef ENABLE_DEBUG_MONITOR
                // Output to monitor for debug
                if (recon_monitor_ == nullptr) {
                    recon_monitor_ = new VideoMonitor(
                        video_src_->get_width_with_padding(),
                        video_src_->get_height_with_padding(),
                        (video_src_->get_bit_depth() > 8)
                            ? video_src_->get_width_with_padding() * 2
                            : video_src_->get_width_with_padding(),
                        video_src_->get_bit_depth(),
                        true,
                        "Recon");
                }
                if (recon_monitor_) {
                    recon_monitor_->draw_frame(mug->mug_buf,
                                               mug->mug_buf + luma_len,
                                               mug->mug_buf + luma_len * 5 / 4);
                }
#endif

                // Do compare image
                ASSERT_EQ(compare_image(mug, &ref_frame), true)
                    << "image compare failed on " << ref_frame.timestamp;

                // Calculate psnr with input frame and
                // recon frame
                uint32_t index = video_src_->get_frame_index();
                EbSvtIOFormat *frame =
                    video_src_->get_frame_by_index(ref_frame.timestamp);
                if (frame) {
                    double luma_psnr = 0.0;
                    double cb_psnr = 0.0;
                    double cr_psnr = 0.0;

                    if (video_src_->get_bit_depth() == 8) {
                        luma_psnr =
                            psnr_8bit(frame->luma, mug->mug_buf, luma_len);
                        cb_psnr = psnr_8bit(
                            frame->cb, mug->mug_buf + luma_len, luma_len >> 2);
                        cr_psnr = psnr_8bit(frame->cr,
                                            mug->mug_buf + luma_len * 5 / 4,
                                            luma_len >> 2);
                    }
                    if (video_src_->get_bit_depth() == 10) {
                        luma_psnr = psnr_10bit((const uint16_t *)frame->luma,
                                               (const uint16_t *)mug->mug_buf,
                                               luma_len / 2);
                        cb_psnr = psnr_10bit(
                            (const uint16_t *)frame->cb,
                            (const uint16_t *)mug->mug_buf + luma_len,
                            luma_len >> 3);
                        cr_psnr = psnr_10bit(
                            (const uint16_t *)frame->cr,
                            (const uint16_t *)mug->mug_buf + luma_len * 5 / 4,
                            luma_len >> 3);
                    }
                    printf(
                        "Do psnr %0.4f, %0.4f, "
                        "%0.f4\r\n",
                        luma_psnr,
                        cb_psnr,
                        cr_psnr);
                }
                video_src_->get_frame_by_index(index);
            }
        }
    };
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
            ASSERT_EQ(recon_frame.n_filled_len, new_mug->mug_size)
                << "recon frame size incorrect@" << recon_frame.pts;
            new_mug->filled_size = recon_frame.n_filled_len;
            new_mug->time_stamp = recon_frame.pts;
            new_mug->tag = recon_frame.flags;
            printf("recon image frame: %lu\n", new_mug->time_stamp);
            recon_sink_->fill_mug(new_mug);
        }
    } while (true);
}

svt_av1_test_e2e::SvtAv1E2ETestFramework::IvfFile::IvfFile(std::string path) {
    FOPEN(file, path.c_str(), "wb");
    byte_count_since_ivf = 0;
    ivf_count = 0;
}
