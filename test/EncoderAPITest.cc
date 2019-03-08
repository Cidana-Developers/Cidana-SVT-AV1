#include "EbApi.h"
#include "gtest/gtest.h"

namespace {

TEST(EncodeAPI, InvalidParams) {
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_encoder(nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_stream_header(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_eos_nal(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_send_picture(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_packet(nullptr, nullptr, 0));
    // EXPECT_EQ(EB_ErrorNone, eb_svt_release_out_buffer(NULL));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_get_recon(nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_deinit_encoder(nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_deinit_handle(nullptr));
}

}  // namespace
