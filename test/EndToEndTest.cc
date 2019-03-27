#include "EbSvtAv1Enc.h"
#include "Y4mVideoSource.h"
#include "YuvVideosource.h"
#include "gtest/gtest.h"

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

static const TestVideoVector video_source_vectors[] = {
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

class EndToEndTest : public ::testing::TestWithParam<TestVideoVector> {
  protected:
    EndToEndTest() : test_vector(GetParam()) {
        video_source = nullptr;
        if (test_vector.file_format == YUM_VIDEO_FILE) {
            video_source = new YuvVideoSource(test_vector.file_name,
                                              test_vector.img_format,
                                              test_vector.width,
                                              test_vector.height,
                                              test_vector.bit_depth);
        }
        if (test_vector.file_format == Y4M_VIDEO_FILE) {
            video_source = new Y4MVideoSource(test_vector.file_name);
        }
    };
    virtual ~EndToEndTest(){};

  public:
    void do_test() {
        EbErrorType return_error = EB_ErrorNone;
        EbComponentType *svt_encoder_handle = nullptr;
        EbSvtAv1EncConfiguration encoder_parameter = {0};
        EbBufferHeaderType *output_stream_buffer = nullptr;
        EbBufferHeaderType *input_picture_buffer = nullptr;
        int32_t frame_count = 0;

        printf("End2End, file:%s\r\n", test_vector.file_name);
        // Check input parameters
        ASSERT_GE(test_vector.width, 0) << "Video vector width error.";
        ASSERT_GE(test_vector.height, 0) << "Video vector height error.";
        ASSERT_LE(test_vector.bit_depth, 10) << "Video vector bitDepth error.";
        ASSERT_GE(test_vector.bit_depth, 8) << "Video vector bitDepth error.";

        //
        // Prepare buffer
        //
        // Input Buffer
        input_picture_buffer =
            (EbBufferHeaderType *)malloc(sizeof(EbBufferHeaderType));
        ASSERT_NE(input_picture_buffer, nullptr)
            << "Malloc memory for inputPictureBuffer failed.";
        input_picture_buffer->p_buffer = nullptr;
        input_picture_buffer->size = sizeof(EbBufferHeaderType);
        input_picture_buffer->p_app_private = nullptr;
        input_picture_buffer->pic_type = EB_AV1_INVALID_PICTURE;

        // Output buffer
        output_stream_buffer =
            (EbBufferHeaderType *)malloc(sizeof(EbBufferHeaderType));
        ASSERT_NE(output_stream_buffer, nullptr)
            << "Malloc memory for outputStreamBuffer failed.";

        output_stream_buffer->p_buffer =
            (uint8_t *)malloc(EB_OUTPUTSTREAMBUFFERSIZE_MACRO(
                test_vector.width * test_vector.height));
        ASSERT_NE(output_stream_buffer->p_buffer, nullptr)
            << "Malloc memory for outputStreamBuffer->p_buffer failed.";

        output_stream_buffer->size = sizeof(EbBufferHeaderType);
        output_stream_buffer->n_alloc_len = EB_OUTPUTSTREAMBUFFERSIZE_MACRO(
            test_vector.width * test_vector.height);
        output_stream_buffer->p_app_private = nullptr;
        output_stream_buffer->pic_type = EB_AV1_INVALID_PICTURE;

        //
        // Init handle & encoder
        //
        return_error =
            eb_init_handle(&svt_encoder_handle, nullptr, &encoder_parameter);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_init_handle return error:" << return_error;
        ASSERT_NE(svt_encoder_handle, nullptr)
            << "eb_init_handle return null handle.";

        encoder_parameter.source_width = test_vector.width;
        encoder_parameter.source_height = test_vector.height;
        encoder_parameter.encoder_bit_depth = test_vector.bit_depth;
        return_error =
            eb_svt_enc_set_parameter(svt_encoder_handle, &encoder_parameter);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_svt_enc_set_parameter return error:" << return_error;

        return_error = eb_init_encoder(svt_encoder_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_init_encoder return error:" << return_error;

        // Get ivf header
        return_error =
            eb_svt_enc_stream_header(svt_encoder_handle, &output_stream_buffer);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_svt_enc_stream_header return error:" << return_error;
        ASSERT_NE(output_stream_buffer, nullptr)
            << "eb_svt_enc_stream_header return null output buffer."
            << return_error;

        return_error == video_source->open_source();
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "Init video source failed, error:" << return_error;
        uint8_t *frame = nullptr;
        do {
            frame = (uint8_t *)video_source->get_next_frame();

            if (frame != nullptr) {
                printf("DISP: %d, size:%d\n",
                       frame_count,
                       video_source->get_frame_size());
                // Fill in Buffers Header control data
                input_picture_buffer->p_buffer = frame;
                input_picture_buffer->n_filled_len =
                    video_source->get_frame_size();
                input_picture_buffer->flags = 0;
                input_picture_buffer->p_app_private = nullptr;
                input_picture_buffer->pts = frame_count++;
                input_picture_buffer->pic_type = EB_AV1_INVALID_PICTURE;
                // Send the picture
                eb_svt_enc_send_picture(svt_encoder_handle,
                                        input_picture_buffer);
            } else {
                EbBufferHeaderType headerPtrLast;
                headerPtrLast.n_alloc_len = 0;
                headerPtrLast.n_filled_len = 0;
                headerPtrLast.n_tick_count = 0;
                headerPtrLast.p_app_private = nullptr;
                headerPtrLast.flags = EB_BUFFERFLAG_EOS;
                headerPtrLast.p_buffer = nullptr;
                input_picture_buffer->flags = EB_BUFFERFLAG_EOS;

                eb_svt_enc_send_picture(svt_encoder_handle, &headerPtrLast);
            }

            // non-blocking call
            EbBufferHeaderType *headerPtr = nullptr;
            return_error = eb_svt_get_packet(
                svt_encoder_handle, &headerPtr, (frame == nullptr ? 1 : 0));
            ASSERT_NE(return_error, EB_ErrorMax)
                << "nError while encoding, code:" << headerPtr->flags;
            // Release the output buffer
            if (headerPtr != nullptr) {
                eb_svt_release_out_buffer(&headerPtr);
            }
        } while ((frame != nullptr));

        // Deinit
        return_error = eb_deinit_encoder(svt_encoder_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_deinit_encoder return error:" << return_error;

        // Destruct the component
        return_error = eb_deinit_handle(svt_encoder_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_deinit_handle return error:" << return_error;

        // Clear
        if (output_stream_buffer != nullptr) {
            if (output_stream_buffer->p_buffer != nullptr) {
                free(output_stream_buffer->p_buffer);
            }
            free(output_stream_buffer);
        }
    }

  private:
    VideoSource *video_source;
    TestVideoVector test_vector;
};

TEST_P(EndToEndTest, EndToEndTest) {
    do_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, EndToEndTest,
                        ::testing::ValuesIn(video_source_vectors));
