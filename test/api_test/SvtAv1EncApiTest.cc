#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"

namespace {

typedef struct {
    EbComponentType* enc_handle;
    EbSvtAv1EncConfiguration enc_params;
} SvtAv1Context;

TEST(EncoderAPI, check_null_pointer) {
    SvtAv1Context context = {0};
    // Test null pointer, expect BadParameters
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));

    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&context.enc_handle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    // TODO(Ryan): Some function will crash with nullptr input,
    // and it will block test on linux platform.
    // EXPECT_EQ(EB_ErrorBadParameter,
    //          eb_svt_enc_set_parameter(context.enc_handle,
    //          nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_encoder(nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_stream_header(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_eos_nal(nullptr, nullptr));
    // EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_send_picture(nullptr,
    // nullptr)); EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_packet(nullptr,
    // nullptr, 0)); EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_recon(nullptr,
    // nullptr));
    // No return value, just feed nullptr as parameter.
    eb_svt_release_out_buffer(nullptr);
    EXPECT_EQ(EB_ErrorBadParameter, eb_deinit_encoder(nullptr));
    EXPECT_EQ(EB_ErrorInvalidComponent, eb_deinit_handle(nullptr));
    SUCCEED();
}

TEST(EncoderAPI, check_normal_setup) {
    SvtAv1Context context = {0};
    const int width = 1280;
    const int height = 720;

    EXPECT_EQ(
        EB_ErrorNone,
        eb_init_handle(
            &context.enc_handle, &context, &context.enc_params))
        << "eb_init_handle failed";
    EXPECT_EQ(EB_ErrorNone,
              eb_svt_enc_set_parameter(context.enc_handle,
                                       &context.enc_params))
        << "eb_svt_enc_set_parameter failed";
    EXPECT_EQ(EB_ErrorNone, eb_init_encoder(context.enc_handle))
        << "eb_init_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_encoder(context.enc_handle))
        << "eb_deinit_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(context.enc_handle))
        << "eb_deinit_handle failed";
}

TEST(EncoderAPI, check_invalid_height) {
    SvtAv1Context context = {0};
    const int width = 1280;
    const int height = 0;

    ASSERT_EQ(
        EB_ErrorNone,
        eb_init_handle(
            &context.enc_handle, &context, &context.enc_params))
        << "eb_init_handle failed";
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_svt_enc_set_parameter(context.enc_handle,
                                       &context.enc_params))
        << "eb_svt_enc_set_parameter return wrong";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(context.enc_handle))
        << "eb_deinit_handle failed";
}

}  // namespace
