/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file SvtAv1EncApiTest.cc
 *
 * @brief SVT-AV1 encoder api test, check invalid input
 *
 * @author Cidana-Edmond, Cidana-Ryan, Cidana-Wenyao
 *
 ******************************************************************************/
#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"
#include "SvtAv1EncApiTest.h"

using namespace svt_av1_test;
/**
 * @brief SVT-AV1 encoder api test
 *
 * Test strategy:
 * Input nullptr to the encoder api and check the return value.
 *
 * Expect result:
 * Encoder should not crash and report EB_ErrorBadParameter.
 *
 * Test coverage:
 * All the encoder parameters.
 */
namespace {

// death tests: TODO: alert, fix me! fix me!! fix me!!!
TEST(EncApiDeathTest, set_parameter_null_pointer) {
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    SvtAv1Context context = {0};

    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&context.enc_handle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    // watch out, someone dead
    ASSERT_DEATH(eb_svt_enc_set_parameter(context.enc_handle, nullptr), "");
    EXPECT_EQ(EB_ErrorInvalidComponent, eb_deinit_handle(nullptr));
    SUCCEED();
}

TEST(EncApiTest, check_null_pointer) {
    SvtAv1Context context = {0};
    // Test null pointer, expect BadParameters
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));

    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&context.enc_handle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    // TODO: Some function will crash with nullptr input,
    // and it will block test on linux platform. please refer to
    // EncApiDeathTest-->check_null_pointer
    // EXPECT_EQ(EB_ErrorBadParameter,
    //          eb_svt_enc_set_parameter(context.enc_handle,
    //          nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_encoder(nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_stream_header(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_eos_nal(nullptr, nullptr));
    // EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_send_picture(nullptr,
    // nullptr)); EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_packet(nullptr,
    // nullptr, 0)); EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_recon(nullptr,
    // nullptr)); No return value, just feed nullptr as parameter.
    eb_svt_release_out_buffer(nullptr);
    EXPECT_EQ(EB_ErrorBadParameter, eb_deinit_encoder(nullptr));
    EXPECT_EQ(EB_ErrorInvalidComponent, eb_deinit_handle(nullptr));
    SUCCEED();
}

TEST(EncApiTest, check_normal_setup) {
    SvtAv1Context context = {0};
    const int width = 1280;
    const int height = 720;

    EXPECT_EQ(
        EB_ErrorNone,
        eb_init_handle(&context.enc_handle, &context, &context.enc_params))
        << "eb_init_handle failed";
    context.enc_params.source_width = width;
    context.enc_params.source_height = height;
    EXPECT_EQ(EB_ErrorNone,
              eb_svt_enc_set_parameter(context.enc_handle, &context.enc_params))
        << "eb_svt_enc_set_parameter failed";
    EXPECT_EQ(EB_ErrorNone, eb_init_encoder(context.enc_handle))
        << "eb_init_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_encoder(context.enc_handle))
        << "eb_deinit_encoder failed";
    EXPECT_EQ(EB_ErrorNone, eb_deinit_handle(context.enc_handle))
        << "eb_deinit_handle failed";
}

TEST(EncApiTest, repeat_normal_setup) {
    SvtAv1Context context = {0};
    const int width = 1280;
    const int height = 720;

    for (size_t i = 0; i < 500; ++i) {
        ASSERT_EQ(
            EB_ErrorNone,
            eb_init_handle(&context.enc_handle, &context, &context.enc_params))
            << "eb_init_handle failed at " << i << " times";
        context.enc_params.source_width = width;
        context.enc_params.source_height = height;
        ASSERT_EQ(
            EB_ErrorNone,
            eb_svt_enc_set_parameter(context.enc_handle, &context.enc_params))
            << "eb_svt_enc_set_parameter failed at " << i << " times";
        // TODO:if not calls eb_deinit_encoder, there is huge memory leak, fix
        // me ASSERT_EQ(EB_ErrorNone, eb_deinit_encoder(context.enc_handle))
        //	<< "eb_deinit_encoder failed";
        ASSERT_EQ(EB_ErrorNone, eb_deinit_handle(context.enc_handle))
            << "eb_deinit_handle failed at " << i << " times";
    }
}

}  // namespace
