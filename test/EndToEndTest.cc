#include "EbApi.h"
#include "gtest/gtest.h"

#define LEFT_INPUT_PADDING 0
#define RIGHT_INPUT_PADDING 0
#define TOP_INPUT_PADDING 0
#define BOTTOM_INPUT_PADDING 0

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

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height, is16bit) \
    ((((width) * (height)*3) >> 1) << is16bit)

typedef struct TestVideoVector_s {
    char *fileName;
    uint32_t width;
    uint32_t height;
    uint8_t bit_depth;
} TestVideoVector_t;

static const TestVideoVector_t video_source_vectors[] = {
    {"../../test/vectors/hantro_collage_w352h288.yuv", 352, 288, 8}};

class EndToEndTest : public ::testing::TestWithParam<TestVideoVector_s> {
  protected:
    EndToEndTest() : test_vector(GetParam()), stop_encoder(false){};
    virtual ~EndToEndTest(){};

  public:
    void do_test() {
        EbErrorType return_error = EB_ErrorNone;
        EbComponentType *svt_encoder_handle = nullptr;
        EbSvtAv1EncConfiguration encoder_parameter = {0};
        EbBufferHeaderType *output_stream_buffer = nullptr;
        EbBufferHeaderType *input_picture_buffer = nullptr;
        int32_t frame_count = 0;
        FILE *video_vector_file = 0;

        // Check input parameters
        video_vector_file = fopen(test_vector.fileName, "rb");
        ASSERT_NE(video_vector_file, nullptr)
            << "Can not open video vector file.";
        ASSERT_GE(test_vector.width, 0) << "Video vector width error.";
        ASSERT_GE(test_vector.height, 0) << "Video vector height error.";
        ASSERT_LE(test_vector.bit_depth, 10) << "Video vector bitDepth error.";
        ASSERT_GE(test_vector.bit_depth, 8) << "Video vector bitDepth error.";

        // Prepare buffer
        // Input Buffer
        input_picture_buffer =
            (EbBufferHeaderType *)malloc(sizeof(EbBufferHeaderType));
        ASSERT_NE(input_picture_buffer, nullptr)
            << "Malloc memory for inputPictureBuffer failed.";

        input_picture_buffer->p_buffer =
            (uint8_t *)malloc(sizeof(EbSvtEncInput));
        ASSERT_NE(input_picture_buffer->p_buffer, nullptr)
            << "Malloc memory for inputPictureBuffer->p_buffer failed.";

        input_picture_buffer->size = sizeof(EbBufferHeaderType);
        input_picture_buffer->p_app_private = NULL;
        input_picture_buffer->pic_type = EB_INVALID_PICTURE;
        // Allocate frame buffer for the p_buffer
        return_error = allocate_fream_buffer(input_picture_buffer->p_buffer);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "AllocateFrameBuffer return error:" << return_error;

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
        output_stream_buffer->p_app_private = NULL;
        output_stream_buffer->pic_type = EB_INVALID_PICTURE;

        // Init handle & encoder
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

        while (stop_encoder == false) {
            read_input_frames(video_vector_file, input_picture_buffer);

            if (stop_encoder == false) {
                // printf ("DISP: %d", frameCount);
                // Fill in Buffers Header control data
                input_picture_buffer->flags = 0;
                input_picture_buffer->p_app_private = NULL;
                input_picture_buffer->pts = frame_count++;
                input_picture_buffer->pic_type = EB_INVALID_PICTURE;
                // Send the picture
                eb_svt_enc_send_picture(svt_encoder_handle,
                                        input_picture_buffer);
            } else {
                EbBufferHeaderType headerPtrLast;
                headerPtrLast.n_alloc_len = 0;
                headerPtrLast.n_filled_len = 0;
                headerPtrLast.n_tick_count = 0;
                headerPtrLast.p_app_private = NULL;
                headerPtrLast.flags = EB_BUFFERFLAG_EOS;
                headerPtrLast.p_buffer = NULL;
                input_picture_buffer->flags = EB_BUFFERFLAG_EOS;

                eb_svt_enc_send_picture(svt_encoder_handle, &headerPtrLast);
            }

            // non-blocking call
            EbBufferHeaderType *headerPtr = nullptr;
            return_error =
                eb_svt_get_packet(svt_encoder_handle, &headerPtr, stop_encoder);
            ASSERT_NE(return_error, EB_ErrorMax)
                << "nError while encoding, code:" << headerPtr->flags;
            // Release the output buffer
            if (headerPtr != nullptr)
                eb_svt_release_out_buffer(&headerPtr);
        }

        // Deinit
        return_error = eb_deinit_encoder(svt_encoder_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_deinit_encoder return error:" << return_error;

        // Destruct the component
        return_error = eb_deinit_handle(svt_encoder_handle);
        ASSERT_EQ(return_error, EB_ErrorNone)
            << "eb_deinit_handle return error:" << return_error;

        // Clear
        // TODO: free buffers
        fclose(video_vector_file);
    }

  private:
    void read_input_frames(FILE *input_file, EbBufferHeaderType *headerPtr) {
        const uint8_t is16bit = (test_vector.bit_depth > 8) ? 1 : 0;
        uint64_t readSize;
        const uint32_t input_padded_width = test_vector.width;
        const uint32_t input_padded_height = test_vector.height;
        uint8_t *eb_input_ptr;
        EbSvtEncInput *input_ptr = (EbSvtEncInput *)headerPtr->p_buffer;
        input_ptr->yStride = input_padded_width;
        input_ptr->cbStride = input_padded_width >> 1;
        input_ptr->crStride = input_padded_width >> 1;
        {
            if (is16bit == 0 /*|| (is16bit == 1 && config->compressedTenBitFormat == 0)*/) {
                readSize = (uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(
                    input_padded_width, input_padded_height, is16bit);

                headerPtr->n_filled_len = 0;

                {
                    uint64_t lumaReadSize =
                        (uint64_t)input_padded_width * input_padded_height
                        << is16bit;
                    eb_input_ptr = input_ptr->luma;
                    headerPtr->n_filled_len += (uint32_t)fread(
                        eb_input_ptr, 1, lumaReadSize, input_file);
                    eb_input_ptr = input_ptr->cb;
                    headerPtr->n_filled_len += (uint32_t)fread(
                        eb_input_ptr, 1, lumaReadSize >> 2, input_file);
                    eb_input_ptr = input_ptr->cr;
                    headerPtr->n_filled_len += (uint32_t)fread(
                        eb_input_ptr, 1, lumaReadSize >> 2, input_file);
                    input_ptr->luma = input_ptr->luma +
                                      ((test_vector.width * TOP_INPUT_PADDING +
                                        LEFT_INPUT_PADDING)
                                       << is16bit);
                    input_ptr->cb =
                        input_ptr->cb +
                        (((test_vector.width >> 1) * (TOP_INPUT_PADDING >> 1) +
                          (LEFT_INPUT_PADDING >> 1))
                         << is16bit);
                    input_ptr->cr =
                        input_ptr->cr +
                        (((test_vector.width >> 1) * (TOP_INPUT_PADDING >> 1) +
                          (LEFT_INPUT_PADDING >> 1))
                         << is16bit);

                    if (readSize != headerPtr->n_filled_len) {
                        stop_encoder = 1;
                    }
                }
            }
            /* TODO: 10bits mode
            // 10-bit Compressed Unpacked Mode
            else if (is16bit == 1 && config->compressedTenBitFormat == 1) {

                    // Fill the buffer with a complete frame
                    headerPtr->n_filled_len = 0;

                    uint64_t lumaReadSize =
            (uint64_t)inputPaddedWidth*inputPaddedHeight; uint64_t
            nbitlumaReadSize = (uint64_t)(inputPaddedWidth /
            4)*inputPaddedHeight;

                    ebInputPtr = inputPtr->luma;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            lumaReadSize, inputFile); ebInputPtr = inputPtr->cb;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            lumaReadSize >> 2, inputFile); ebInputPtr = inputPtr->cr;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            lumaReadSize >> 2, inputFile);

                    inputPtr->luma = inputPtr->luma +
            config->inputPaddedWidth*TOP_INPUT_PADDING + LEFT_INPUT_PADDING;
                    inputPtr->cb = inputPtr->cb + (config->inputPaddedWidth >>
            1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);
                    inputPtr->cr = inputPtr->cr + (config->inputPaddedWidth >>
            1)*(TOP_INPUT_PADDING >> 1) + (LEFT_INPUT_PADDING >> 1);


                    ebInputPtr = inputPtr->lumaExt;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            nbitlumaReadSize, inputFile); ebInputPtr = inputPtr->cbExt;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            nbitlumaReadSize >> 2, inputFile); ebInputPtr = inputPtr->crExt;
                    headerPtr->n_filled_len += (uint32_t)fread(ebInputPtr, 1,
            nbitlumaReadSize >> 2, inputFile);

                    inputPtr->lumaExt = inputPtr->lumaExt +
            ((config->inputPaddedWidth >> 2)*TOP_INPUT_PADDING +
            (LEFT_INPUT_PADDING >> 2)); inputPtr->cbExt = inputPtr->cbExt +
            (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) +
            ((LEFT_INPUT_PADDING >> 1) >> 2)); inputPtr->crExt = inputPtr->crExt
            + (((config->inputPaddedWidth >> 1) >> 2)*(TOP_INPUT_PADDING >> 1) +
            ((LEFT_INPUT_PADDING >> 1) >> 2));

                    readSize = ((lumaReadSize * 3) >> 1) + ((nbitlumaReadSize *
            3) >> 1);

                    if (readSize != headerPtr->n_filled_len) {
                            config->stopEncoder = 1;
                    }

            }
            */
        }
        // If we reached the end of file, loop over again
        if (feof(input_file) != 0) {
            stop_encoder = 1;
        }

        return;
    }

    EbErrorType allocate_fream_buffer(uint8_t *p_buffer) {
        EbErrorType return_error = EB_ErrorNone;
        // TODO: check tenBitPackedMode?
        const int32_t ten_bits_packed_mode =
            (test_vector.bit_depth > 8 /*&& tenBitPackedMode == 0*/) ? 1 : 0;

        // Determine size of each plane
        const size_t luma_8bit_size = test_vector.width * test_vector.height *
                                      (1 << ten_bits_packed_mode);

        const size_t chroma8bitSize = luma_8bit_size >> 2;
        // TODO: check tenBitPackedMode?
        const size_t luma10bitSize =
            (test_vector.bit_depth > 8 /*&& tenBitPackedMode == 0*/)
                ? luma_8bit_size
                : 0;
        const size_t chroma10bitSize =
            (test_vector.bit_depth > 8 /*&& tenBitPackedMode == 0*/)
                ? chroma8bitSize
                : 0;

        // Determine
        EbSvtEncInput *inputPtr = (EbSvtEncInput *)p_buffer;

        if (luma_8bit_size) {
            inputPtr->luma = (uint8_t *)malloc(luma_8bit_size);
            if (!inputPtr->luma)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->luma = 0;
        }
        if (chroma8bitSize) {
            inputPtr->cb = (uint8_t *)malloc(chroma8bitSize);
            if (!inputPtr->cb)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->cb = 0;
        }
        if (chroma8bitSize) {
            inputPtr->cr = (uint8_t *)malloc(chroma8bitSize);
            if (!inputPtr->cr)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->cr = 0;
        }
        if (luma10bitSize) {
            inputPtr->lumaExt = (uint8_t *)malloc(luma10bitSize);
            if (!inputPtr->lumaExt)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->lumaExt = 0;
        }
        if (chroma10bitSize) {
            inputPtr->cbExt = (uint8_t *)malloc(chroma10bitSize);
            if (!inputPtr->cbExt)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->cbExt = 0;
        }
        if (chroma10bitSize) {
            inputPtr->crExt = (uint8_t *)malloc(chroma10bitSize);
            if (!inputPtr->crExt)
                return EB_ErrorInsufficientResources;
        } else {
            inputPtr->crExt = 0;
        }
        return return_error;
    }

    TestVideoVector_s test_vector;
    bool stop_encoder;
};

TEST_P(EndToEndTest, EndToEndTest) {
    do_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, EndToEndTest,
                        ::testing::ValuesIn(video_source_vectors));
