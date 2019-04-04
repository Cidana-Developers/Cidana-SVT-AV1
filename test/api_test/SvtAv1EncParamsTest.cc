/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file SvtAv1EncParamsTest.cc
 *
 * @brief SVT-AV1 encoder parameter configuration test
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"
#include "params.h"
#include "SvtAv1EncApiTest.h"

using namespace svt_av1_test_params;
using namespace svt_av1_test;

namespace {
/**
 * @brief SVT-AV1 encoder parameter configuration test
 *
 * Test strategy:
 * Feed default values, vaild values and invalid values of individual param
 * to the encoder and check if encoder return correctly.
 *
 * Expect result:
 * For default value and valid values, encoder should return EB_ErrorNone.
 * For invalid value, encoder should return EB_ErrorBadParameter.
 *
 * Test coverage:
 * Almost all the encoder parameters except frame_rate_numerator and
 * frame_rate_denominator
 */
class EncParamTestBase : public ::testing::Test {
  public:
    EncParamTestBase() {
        memset(&ctxt_, 0, sizeof(ctxt_));
        param_name_str_ = "";
    }
    EncParamTestBase(std::string param_name) {
        memset(&ctxt_, 0, sizeof(ctxt_));
        param_name_str_ = param_name;
    }
    virtual ~EncParamTestBase() {
    }
    virtual void run_default_param_check() = 0;
    virtual void run_valid_param_check() = 0;
    virtual void run_invalid_param_check() = 0;
    virtual void run_special_param_check() = 0;

  protected:
    // Sets up the test fixture.
    virtual void SetUp() override {
        ASSERT_EQ(EB_ErrorNone,
                  eb_init_handle(&ctxt_.enc_handle, &ctxt_, &ctxt_.enc_params))
            << "eb_init_handle failed";
        ASSERT_NE(nullptr, ctxt_.enc_handle) << "enc_handle is invalid";
        if (strcmp(param_name_str_.c_str(), "source_width")) {
            const int width = 1280;
            ctxt_.enc_params.source_width = width;
        }
        if (strcmp(param_name_str_.c_str(), "source_height")) {
            const int height = 720;
            ctxt_.enc_params.source_height = height;
        }
    }

    // Tears down the test fixture.
    virtual void TearDown() override {
        // TODO: eb_deinit_encoder should not be called here, for this test does
        // not call eb_init_encoder, but there is huge memory leak if only calls
        // eb_deinit_handle. please remmove it after we pass
        // EncApiTest-->repeat_normal_setup
        ASSERT_EQ(EB_ErrorNone, eb_deinit_encoder(ctxt_.enc_handle))
            << "eb_deinit_encoder failed";
        ASSERT_EQ(EB_ErrorNone, eb_deinit_handle(ctxt_.enc_handle))
            << "eb_deinit_handle failed";
    }

  protected:
    SvtAv1Context ctxt_;
    std::string param_name_str_;
};

#define PRINT_PARAM_FATAL(p) \
    << "eb_svt_enc_set_parameter " << #p << ": " << (int)(p) << " failed"

#define PRINT_2PARAM_FATAL(p1, p2)                                             \
    << "eb_svt_enc_set_parameter " << #p1 << ": " << (int)(p1) << " + " << #p2 \
    << ": " << (int)(p2) << " failed"

#define PARAM_TEST(param_test)               \
    TEST_F(param_test, run_paramter_check) { \
        run_default_param_check();           \
        run_valid_param_check();             \
        run_invalid_param_check();           \
        run_special_param_check();           \
    }

#define DEFINE_PARAM_TEST_CLASS(test_name, param_name)                        \
    class test_name : public EncParamTestBase {                               \
      public:                                                                 \
        test_name() : EncParamTestBase(#param_name) {                         \
        }                                                                     \
        virtual void run_default_param_check() override {                     \
            EncParamTestBase::SetUp();                                        \
            ASSERT_EQ(ctxt_.enc_params.param_name,                            \
                      GET_DEFAULT_PARAM(param_name));                         \
            EncParamTestBase::TearDown();                                     \
        }                                                                     \
        virtual void run_valid_param_check() override {                       \
            for (size_t i = 0; i < SIZE_VALID_PARAM(param_name); ++i) {       \
                EncParamTestBase::SetUp();                                    \
                ctxt_.enc_params.param_name = GET_VALID_PARAM(param_name, i); \
                EXPECT_EQ(EB_ErrorNone,                                       \
                          eb_svt_enc_set_parameter(ctxt_.enc_handle,          \
                                                   &ctxt_.enc_params))        \
                PRINT_PARAM_FATAL(ctxt_.enc_params.param_name);               \
                EncParamTestBase::TearDown();                                 \
            }                                                                 \
        }                                                                     \
        virtual void run_invalid_param_check() override {                     \
            for (size_t i = 0; i < SIZE_INVALID_PARAM(param_name); ++i) {     \
                EncParamTestBase::SetUp();                                    \
                ctxt_.enc_params.param_name =                                 \
                    GET_INVALID_PARAM(param_name, i);                         \
                EXPECT_EQ(EB_ErrorBadParameter,                               \
                          eb_svt_enc_set_parameter(ctxt_.enc_handle,          \
                                                   &ctxt_.enc_params))        \
                PRINT_PARAM_FATAL(ctxt_.enc_params.param_name);               \
                EncParamTestBase::TearDown();                                 \
            }                                                                 \
        }                                                                     \
        virtual void run_special_param_check() override {                     \
            /*do nothing for special cases*/                                  \
        }                                                                     \
                                                                              \
      protected:                                                              \
        virtual void SetUp() override {                                       \
            /* skip EncParamTestBase::SetUp() */                              \
        }                                                                     \
        virtual void TearDown() override {                                    \
            /* skip EncParamTestBase::TearDown() */                           \
        }                                                                     \
    };

DEFINE_PARAM_TEST_CLASS(EncParamEncModeTest, enc_mode);
PARAM_TEST(EncParamEncModeTest);

DEFINE_PARAM_TEST_CLASS(EncParamIntraPeridLenTest, intra_period_length);
PARAM_TEST(EncParamIntraPeridLenTest);

DEFINE_PARAM_TEST_CLASS(EncParamIntraRefreshTypeTest, intra_refresh_type);
PARAM_TEST(EncParamIntraRefreshTypeTest);

DEFINE_PARAM_TEST_CLASS(EncParamHierarchicalLvlTest, hierarchical_levels);
PARAM_TEST(EncParamHierarchicalLvlTest);

DEFINE_PARAM_TEST_CLASS(EncParamPredStructTest, pred_structure);
PARAM_TEST(EncParamPredStructTest);

DEFINE_PARAM_TEST_CLASS(EncParamSrcWidthTest, source_width);
PARAM_TEST(EncParamSrcWidthTest);

DEFINE_PARAM_TEST_CLASS(EncParamSrcHeightTest, source_height);
PARAM_TEST(EncParamSrcHeightTest);

DEFINE_PARAM_TEST_CLASS(EncParamFrameRateTest, frame_rate);
PARAM_TEST(EncParamFrameRateTest);

DEFINE_PARAM_TEST_CLASS(EncParamEncBitDepthTest, encoder_bit_depth);
PARAM_TEST(EncParamEncBitDepthTest);

DEFINE_PARAM_TEST_CLASS(EncParamCompr10BitFmtTest, compressed_ten_bit_format);
PARAM_TEST(EncParamCompr10BitFmtTest);

DEFINE_PARAM_TEST_CLASS(EncParamFrame2EncTest, frames_to_be_encoded);
PARAM_TEST(EncParamFrame2EncTest);

DEFINE_PARAM_TEST_CLASS(EncParamImproveSharpTest, improve_sharpness);
PARAM_TEST(EncParamImproveSharpTest);

DEFINE_PARAM_TEST_CLASS(EncParamSbSizeTest, sb_sz);
PARAM_TEST(EncParamSbSizeTest);

DEFINE_PARAM_TEST_CLASS(EncParamSuperBlockSizeTest, super_block_size);
PARAM_TEST(EncParamSuperBlockSizeTest);

DEFINE_PARAM_TEST_CLASS(EncParamPartitionDepthTest, partition_depth);
PARAM_TEST(EncParamPartitionDepthTest);

DEFINE_PARAM_TEST_CLASS(EncParamQPTest, qp);
PARAM_TEST(EncParamQPTest);

DEFINE_PARAM_TEST_CLASS(EncParamUseQPFileTest, use_qp_file);
PARAM_TEST(EncParamUseQPFileTest);

DEFINE_PARAM_TEST_CLASS(EncParamEnableQPScaleTest, enable_qp_scaling_flag);
PARAM_TEST(EncParamEnableQPScaleTest);

DEFINE_PARAM_TEST_CLASS(EncParamDisableDlfTest, disable_dlf_flag);
PARAM_TEST(EncParamDisableDlfTest);

DEFINE_PARAM_TEST_CLASS(EncParamEnableDenoiseTest, enable_denoise_flag);
PARAM_TEST(EncParamEnableDenoiseTest);

DEFINE_PARAM_TEST_CLASS(EncParamFilmGrainDenoiseStrTest,
                        film_grain_denoise_strength);
PARAM_TEST(EncParamFilmGrainDenoiseStrTest);

DEFINE_PARAM_TEST_CLASS(EncParamEnableWarpedMotionTest, enable_warped_motion);
PARAM_TEST(EncParamEnableWarpedMotionTest);

DEFINE_PARAM_TEST_CLASS(EncParamUseDefaultMeHmeTest, use_default_me_hme);
PARAM_TEST(EncParamUseDefaultMeHmeTest);

DEFINE_PARAM_TEST_CLASS(EncParamEnableHmeTest, enable_hme_flag);
PARAM_TEST(EncParamEnableHmeTest);

DEFINE_PARAM_TEST_CLASS(EncParamExtBlockTest, ext_block_flag);
PARAM_TEST(EncParamExtBlockTest);

DEFINE_PARAM_TEST_CLASS(EncParamInLoopMeTest, in_loop_me_flag);
PARAM_TEST(EncParamInLoopMeTest);

DEFINE_PARAM_TEST_CLASS(EncParamSearchAreaWidthTest, search_area_width);
PARAM_TEST(EncParamSearchAreaWidthTest);

DEFINE_PARAM_TEST_CLASS(EncParamSearchAreaHeightTest, search_area_height);
PARAM_TEST(EncParamSearchAreaHeightTest);

DEFINE_PARAM_TEST_CLASS(EncParamConstrainedIntraTest, constrained_intra);
PARAM_TEST(EncParamConstrainedIntraTest);

DEFINE_PARAM_TEST_CLASS(EncParamRateCtrlModeTest, rate_control_mode);
PARAM_TEST(EncParamRateCtrlModeTest);

DEFINE_PARAM_TEST_CLASS(EncParamSceneChangeDectTest, scene_change_detection);
PARAM_TEST(EncParamSceneChangeDectTest);

DEFINE_PARAM_TEST_CLASS(EncParamLookAheadDistanceTest, look_ahead_distance);
PARAM_TEST(EncParamLookAheadDistanceTest);

DEFINE_PARAM_TEST_CLASS(EncParamTargetBitRateTest, target_bit_rate);
PARAM_TEST(EncParamTargetBitRateTest);

DEFINE_PARAM_TEST_CLASS(EncParamMaxQPAllowTest, max_qp_allowed);
PARAM_TEST(EncParamMaxQPAllowTest);

DEFINE_PARAM_TEST_CLASS(EncParamMinQPAllowTest, min_qp_allowed);
PARAM_TEST(EncParamMinQPAllowTest);

DEFINE_PARAM_TEST_CLASS(EncParamHighDynamicRangeInputTest,
                        high_dynamic_range_input);
PARAM_TEST(EncParamHighDynamicRangeInputTest);

DEFINE_PARAM_TEST_CLASS(EncParamProfileTest, profile);
PARAM_TEST(EncParamProfileTest);

DEFINE_PARAM_TEST_CLASS(EncParamTierTest, tier);
PARAM_TEST(EncParamTierTest);

DEFINE_PARAM_TEST_CLASS(EncParamLevelTest, level);
PARAM_TEST(EncParamLevelTest);

DEFINE_PARAM_TEST_CLASS(EncParamAsmTypeTest, asm_type);
PARAM_TEST(EncParamAsmTypeTest);

DEFINE_PARAM_TEST_CLASS(EncParamChIdTest, channel_id);
PARAM_TEST(EncParamChIdTest);

DEFINE_PARAM_TEST_CLASS(EncParamActiveChCountTest, active_channel_count);
PARAM_TEST(EncParamActiveChCountTest);

DEFINE_PARAM_TEST_CLASS(EncParamSpeedCtrlTest, speed_control_flag);
PARAM_TEST(EncParamSpeedCtrlTest);

DEFINE_PARAM_TEST_CLASS(EncParamInjectorFrameRateTest, injector_frame_rate);
PARAM_TEST(EncParamInjectorFrameRateTest);

DEFINE_PARAM_TEST_CLASS(EncParamLogicalProcessorsTest, logical_processors);
PARAM_TEST(EncParamLogicalProcessorsTest);

DEFINE_PARAM_TEST_CLASS(EncParamTargetSocketTest, target_socket);
PARAM_TEST(EncParamTargetSocketTest);

DEFINE_PARAM_TEST_CLASS(EncParamReconEnabledTest, recon_enabled);
PARAM_TEST(EncParamReconEnabledTest);

#if TILES
DEFINE_PARAM_TEST_CLASS(EncParamTileColsTest, tile_columns);
PARAM_TEST(EncParamTileColsTest);

DEFINE_PARAM_TEST_CLASS(EncParamTileRowsTest, tile_rows);
PARAM_TEST(EncParamTileRowsTest);
#endif
}  // namespace
