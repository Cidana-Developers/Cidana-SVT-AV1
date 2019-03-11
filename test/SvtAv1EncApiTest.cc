#include "gtest/gtest.h"
extern "C" {
#include "EbAppConfig.h"
#include "EbAppContext.h"
}
#include "EbApi.h"

// TODO(wenyao): This declaration is copied from
// EbAppContext.c, since this function is not
// declared in EbAppContext.h.
/***********************************************
 * Copy configuration parameters from
 *  The config structure, to the
 *  callback structure to send to the library
 ***********************************************/
extern "C" EbErrorType CopyConfigurationParameters(EbConfig_t *config,
                                                   EbAppContext_t *callbackData,
                                                   uint32_t instanceIdx);

TEST(EncoderAPI, NullPointParam) {
    // testing::internal::CaptureStdout();
    EbAppContext_t appCallback = {0};
    // Test null pointer, expect BadParameters
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));

    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&appCallback.svtEncoderHandle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_svt_enc_set_parameter(appCallback.svtEncoderHandle, nullptr));
    EXPECT_EQ(/*EB_ErrorBadParameter*/ EB_ErrorNone, eb_init_encoder(nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_deinit_encoder(nullptr));
    EXPECT_EQ(EB_ErrorInvalidComponent, eb_deinit_handle(nullptr));
    // std::string output = testing::internal::GetCapturedStdout();
    SUCCEED();
}

TEST(EncoderAPI, NormalSetup) {
    EbAppContext_t appCallback = {0};
    EbConfig_t config = {0};
    int width = 1280;
    int height = 720;

    // Test normal setup
    // TODO(wenyao): Test other resolution supported by
    // SVT-AV1
    EbConfigCtor(&config);
    config.inputPaddedWidth = config.sourceWidth = width;
    config.inputPaddedHeight = config.sourceHeight = height;
    config.encoderBitDepth = 8;

    EXPECT_EQ(EB_ErrorNone,
              eb_init_handle(&appCallback.svtEncoderHandle,
                             &appCallback,
                             &appCallback.ebEncParameters))
        << "eb_init_handle failed";
    CopyConfigurationParameters(&config, &appCallback, 0);
    EXPECT_EQ(EB_ErrorNone,
              eb_svt_enc_set_parameter(appCallback.svtEncoderHandle,
                                       &appCallback.ebEncParameters))
        << "eb_svt_enc_set_parameter failed";
    EXPECT_EQ(EB_ErrorNone, eb_init_encoder(appCallback.svtEncoderHandle))
        << "eb_init_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_encoder(appCallback.svtEncoderHandle))
        << "eb_deinit_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(appCallback.svtEncoderHandle))
        << "eb_deinit_handle failed";
}

TEST(EncoderAPI, InvalidHeight) {
    EbAppContext_t appCallback = {0};
    EbConfig_t config = {0};
    int width = 1280;
    int height = 0;

    // Test normal configuration
    // TODO(wenyao): Test other resolution supported by
    // SVT-AV1
    EbConfigCtor(&config);
    config.inputPaddedWidth = config.sourceWidth = width;
    config.inputPaddedHeight = config.sourceHeight = height;
    config.encoderBitDepth = 8;

    ASSERT_EQ(EB_ErrorNone,
              eb_init_handle(&appCallback.svtEncoderHandle,
                             &appCallback,
                             &appCallback.ebEncParameters))
        << "eb_init_handle failed";
    CopyConfigurationParameters(&config, &appCallback, 0);
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_svt_enc_set_parameter(appCallback.svtEncoderHandle,
                                       &appCallback.ebEncParameters))
        << "eb_svt_enc_set_parameter return wrong";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(appCallback.svtEncoderHandle))
        << "eb_deinit_handle failed";
}
