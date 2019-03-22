#include "EbApi.h"
#include "gtest/gtest.h"

namespace {

typedef struct {
    EbComponentType* svtEncoderHandle;
    EbSvtAv1EncConfiguration ebEncParameters;
} SvtAv1Context;

TEST(EncoderAPI, NullPointParam) {
    SvtAv1Context context = {0};
    // Test null pointer, expect BadParameters
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));

    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&context.svtEncoderHandle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    // TODO(Ryan): Some function will crash with nullptr input,
    // and it will block test on linux platform.
    // EXPECT_EQ(EB_ErrorBadParameter,
    //          eb_svt_enc_set_parameter(context.svtEncoderHandle,
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

TEST(EncoderAPI, NormalSetup) {
    SvtAv1Context context = {0};
    int width = 1280;
    int height = 720;

    EXPECT_EQ(
        EB_ErrorNone,
        eb_init_handle(
            &context.svtEncoderHandle, &context, &context.ebEncParameters))
        << "eb_init_handle failed";
    EXPECT_EQ(EB_ErrorNone,
              eb_svt_enc_set_parameter(context.svtEncoderHandle,
                                       &context.ebEncParameters))
        << "eb_svt_enc_set_parameter failed";
    EXPECT_EQ(EB_ErrorNone, eb_init_encoder(context.svtEncoderHandle))
        << "eb_init_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_encoder(context.svtEncoderHandle))
        << "eb_deinit_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(context.svtEncoderHandle))
        << "eb_deinit_handle failed";
}

TEST(EncoderAPI, InvalidHeight) {
    SvtAv1Context context = {0};
    int width = 1280;
    int height = 0;

    ASSERT_EQ(
        EB_ErrorNone,
        eb_init_handle(
            &context.svtEncoderHandle, &context, &context.ebEncParameters))
        << "eb_init_handle failed";
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_svt_enc_set_parameter(context.svtEncoderHandle,
                                       &context.ebEncParameters))
        << "eb_svt_enc_set_parameter return wrong";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(context.svtEncoderHandle))
        << "eb_deinit_handle failed";
}

}  // namespace
