/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file SvtAv1E2EParamsTest.cc
 *
 * @brief Impelmentation of encoder parameter coverage test in E2E test
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"
#include "../api_test/params.h"

#define HIGH_LEVEL_TOOL 0
#define LOW_LEVEL_TOOL 0

/**
 * @brief SVT-AV1 encoder parameter coverage E2E test
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with individual parameter in vaild value and run the
 * comformance test progress to check when the result can match the output of
 * refence decoder
 *
 * Expect result:
 * No error from encoding progress and the reconstruction frame is same as the
 * output frame from refence decoder
 *
 * Test coverage:
 * Almost all the encoder parameters except frame_rate_numerator and
 * frame_rate_denominator
 */

using namespace svt_av1_e2e_test;
using namespace svt_av1_e2e_test_vector;

/** Marcro defininition of batch processing check for default, valid, invalid
 * and special parameter check*/
#define PARAM_TEST_WITH_VECTOR(param_test, vectors)     \
    TEST_P(param_test, run_paramter_conformance_test) { \
        run_conformance_test();                         \
    }                                                   \
    INSTANTIATE_TEST_CASE_P(SVT_AV1, param_test, ::testing::ValuesIn(vectors));

#define PARAM_TEST(param_test) \
    PARAM_TEST_WITH_VECTOR(param_test, smoking_vectors)

/** @breif This class is a template based on EncParamTestBase to test each
 * parameter
 */
#define DEFINE_PARAM_TEST_CLASS(test_name, param_name)                        \
    class test_name : public SvtAv1E2ETestFramework {                         \
      public:                                                                 \
        test_name() {                                                         \
            param_name_str_ = #param_name;                                    \
            param_value_idx_ = 0;                                             \
        }                                                                     \
        /** initialization for test */                                        \
        void init_test() override {                                           \
            collect_ = new PerformanceCollect(typeid(this).name());           \
            av1enc_ctx_.enc_params.param_name =                               \
                GET_VALID_PARAM(param_name, param_value_idx_);                \
            /** create recon sink before setup parameter of encoder */        \
            VideoFrameParam param;                                            \
            memset(&param, 0, sizeof(param));                                 \
            param.format = video_src_->get_image_format();                    \
            param.width = video_src_->get_width_with_padding();               \
            param.height = video_src_->get_height_with_padding();             \
            recon_sink_ = create_recon_sink(param);                           \
            ASSERT_NE(recon_sink_, nullptr) << "can not create recon sink!!"; \
            if (recon_sink_)                                                  \
                av1enc_ctx_.enc_params.recon_enabled = 1;                     \
                                                                              \
            /** create reference decoder*/                                    \
            refer_dec_ = create_reference_decoder();                          \
            ASSERT_NE(refer_dec_, nullptr)                                    \
                << "can not create reference decoder!!";                      \
            pre_process_param();                                              \
            SvtAv1E2ETestFramework::init_test();                              \
        }                                                                     \
        /** close for test */                                                 \
        void close_test() override {                                          \
            post_process_param();                                             \
            SvtAv1E2ETestFramework::close_test();                             \
            if (collect_) {                                                   \
                delete collect_;                                              \
                collect_ = nullptr;                                           \
            }                                                                 \
        }                                                                     \
        /** run for the conformance test */                                   \
        void run_conformance_test() {                                         \
            for (param_value_idx_ = 0;                                        \
                 param_value_idx_ < SIZE_VALID_PARAM(param_name);             \
                 ++param_value_idx_) {                                        \
                SvtAv1E2ETestFramework::SetUp();                              \
                run_encode_process();                                         \
                SvtAv1E2ETestFramework::TearDown();                           \
            }                                                                 \
        }                                                                     \
        /** setup some of the params with related params modified before set  \
         * to encoder */                                                      \
        void pre_process_param() {                                            \
            if (!strcmp(param_name_str_.c_str(),                              \
                        "film_grain_denoise_strength")) {                     \
                av1enc_ctx_.enc_params.enable_denoise_flag = 1;               \
            }                                                                 \
            if (!strcmp(param_name_str_.c_str(), "target_bit_rate")) {        \
                av1enc_ctx_.enc_params.rate_control_mode = 1;                 \
            }                                                                 \
            if (!strcmp(param_name_str_.c_str(), "injector_frame_rate")) {    \
                av1enc_ctx_.enc_params.speed_control_flag = 1;                \
            }                                                                 \
            if (!strcmp(param_name_str_.c_str(), "profile")) {                \
                if (av1enc_ctx_.enc_params.profile == 1) {                    \
                    /** profile(1) requires 8-bit YUV444 */                   \
                    ASSERT_EQ(av1enc_ctx_.enc_params.encoder_bit_depth, 8);   \
                    ASSERT_EQ(av1enc_ctx_.enc_params.encoder_color_format,    \
                              EB_YUV444);                                     \
                }                                                             \
                if (av1enc_ctx_.enc_params.profile == 2) {                    \
                    /** profile(2) requires 8-bit/10-bit YUV422 */            \
                    ASSERT_GE(av1enc_ctx_.enc_params.encoder_bit_depth, 8);   \
                    ASSERT_LE(av1enc_ctx_.enc_params.encoder_bit_depth, 10);  \
                    ASSERT_EQ(av1enc_ctx_.enc_params.encoder_color_format,    \
                              EB_YUV422);                                     \
                }                                                             \
            }                                                                 \
        }                                                                     \
        /** additional process after parameter test finish */                 \
        void post_process_param() {                                           \
            /** TODO: add process after test by params*/                      \
        }                                                                     \
                                                                              \
      protected:                                                              \
        void SetUp() override {                                               \
            /* skip SvtAv1E2ETestFramework::SetUp() */                        \
        }                                                                     \
        void TearDown() override {                                            \
            /* skip SvtAv1E2ETestFramework::TearDown() */                     \
        }                                                                     \
                                                                              \
      protected:                                                              \
        std::string param_name_str_; /**< name of parameter for test */       \
        size_t param_value_idx_; /**< index of parameter value in vector */   \
    };

/** Test case for enc_mode*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEncModeTest, enc_mode);
PARAM_TEST(SvtAv1E2EParamEncModeTest);

#if HIGH_LEVEL_TOOL
/** Test case for intra_period_length*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamIntraPeridLenTest, intra_period_length);
PARAM_TEST(SvtAv1E2EParamIntraPeridLenTest);

/** Test case for intra_refresh_type*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamIntraRefreshTypeTest, intra_refresh_type);
PARAM_TEST(SvtAv1E2EParamIntraRefreshTypeTest);

/** Test case for hierarchical_levels*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamHierarchicalLvlTest, hierarchical_levels);
PARAM_TEST(SvtAv1E2EParamHierarchicalLvlTest);

/** Test case for pred_structure*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamPredStructTest, pred_structure);
PARAM_TEST(SvtAv1E2EParamPredStructTest);
#endif  // HIGH_LEVEL_TOOL

/** Test case for
 * source_width <br>
 * source_height <br>
 * encoder_bit_depth <br>
 * compressed_ten_bit_format <br>
 * recon_enabled <br>
 * are covered by conformance test*/

/** Test case for frame_rate*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamFrameRateTest, frame_rate);
PARAM_TEST(SvtAv1E2EParamFrameRateTest);

/** Test case for frames_to_be_encoded*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamFrame2EncTest, frames_to_be_encoded);
PARAM_TEST(SvtAv1E2EParamFrame2EncTest);

/** Test case for improve_sharpness*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamImproveSharpTest, improve_sharpness);
PARAM_TEST(SvtAv1E2EParamImproveSharpTest);

#if LOW_LEVEL_TOOL
/** Test case for sb_sz*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSbSizeTest, sb_sz);
PARAM_TEST(SvtAv1E2EParamSbSizeTest);
#endif  // LOW_LEVEL_TOOL

#if HIGH_LEVEL_TOOL
/** Test case for super_block_size*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSuperBlockSizeTest, super_block_size);
PARAM_TEST(SvtAv1E2EParamSuperBlockSizeTest);
#endif  // HIGH_LEVEL_TOOL

#if LOW_LEVEL_TOOL
/** Test case for partition_depth*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamPartitionDepthTest, partition_depth);
PARAM_TEST(SvtAv1E2EParamPartitionDepthTest);
#endif  // LOW_LEVEL_TOOL

#if HIGH_LEVEL_TOOL
/** Test case for qp*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamQPTest, qp);
PARAM_TEST(SvtAv1E2EParamQPTest);
#endif  // HIGH_LEVEL_TOOL

#if LOW_LEVEL_TOOL
/** Test case for use_qp_file*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamUseQPFileTest, use_qp_file);
PARAM_TEST(SvtAv1E2EParamUseQPFileTest);

/** Test case for enable_qp_scaling_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableQPScaleTest,
                        enable_qp_scaling_flag);
PARAM_TEST(SvtAv1E2EParamEnableQPScaleTest);
#endif  // LOW_LEVEL_TOOL

#if HIGH_LEVEL_TOOL
/** Test case for disable_dlf_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamDisableDlfTest, disable_dlf_flag);
PARAM_TEST(SvtAv1E2EParamDisableDlfTest);
#endif  // HIGH_LEVEL_TOOL

/** Test case for enable_denoise_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableDenoiseTest, enable_denoise_flag);
PARAM_TEST(SvtAv1E2EParamEnableDenoiseTest);

/** Test case for film_grain_denoise_strength*/
#if THIS_TEST_IS_DEATH
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamFilmGrainDenoiseStrTest,
                        film_grain_denoise_strength);
PARAM_TEST(SvtAv1E2EParamFilmGrainDenoiseStrTest);
#endif  // THIS_TEST_IS_DEATH

#if LOW_LEVEL_TOOL
/** Test case for enable_warped_motion*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableWarpedMotionTest,
                        enable_warped_motion);
PARAM_TEST(SvtAv1E2EParamEnableWarpedMotionTest);
#endif  // LOW_LEVEL_TOOL

/** Test case for use_default_me_hme*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamUseDefaultMeHmeTest, use_default_me_hme);
PARAM_TEST(SvtAv1E2EParamUseDefaultMeHmeTest);

/** Test case for enable_hme_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableHmeTest, enable_hme_flag);
PARAM_TEST(SvtAv1E2EParamEnableHmeTest);

#if LOW_LEVEL_TOOL
/** Test case for ext_block_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamExtBlockTest, ext_block_flag);
PARAM_TEST(SvtAv1E2EParamExtBlockTest);
#endif  // LOW_LEVEL_TOOL

/** Test case for in_loop_me_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamInLoopMeTest, in_loop_me_flag);
PARAM_TEST(SvtAv1E2EParamInLoopMeTest);

/** Test case for search_area_width*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSearchAreaWidthTest, search_area_width);
PARAM_TEST(SvtAv1E2EParamSearchAreaWidthTest);

/** Test case for search_area_height*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSearchAreaHeightTest, search_area_height);
PARAM_TEST(SvtAv1E2EParamSearchAreaHeightTest);

/** Test case for constrained_intra*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamConstrainedIntraTest, constrained_intra);
PARAM_TEST(SvtAv1E2EParamConstrainedIntraTest);

#if LOW_LEVEL_TOOL
/** Test case for rate_control_mode*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamRateCtrlModeTest, rate_control_mode);
PARAM_TEST(SvtAv1E2EParamRateCtrlModeTest);
#endif  // LOW_LEVEL_TOOL

#if HIGH_LEVEL_TOOL
/** Test case for scene_change_detection*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSceneChangeDectTest,
                        scene_change_detection);
PARAM_TEST(SvtAv1E2EParamSceneChangeDectTest);

/** Test case for look_ahead_distance*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamLookAheadDistanceTest,
                        look_ahead_distance);
PARAM_TEST(SvtAv1E2EParamLookAheadDistanceTest);
#endif  // HIGH_LEVEL_TOOL

/** Test case for target_bit_rate*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTargetBitRateTest, target_bit_rate);
PARAM_TEST(SvtAv1E2EParamTargetBitRateTest);

#if LOW_LEVEL_TOOL
/** Test case for max_qp_allowed*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamMaxQPAllowTest, max_qp_allowed);
PARAM_TEST(SvtAv1E2EParamMaxQPAllowTest);

/** Test case for min_qp_allowed*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamMinQPAllowTest, min_qp_allowed);
PARAM_TEST(SvtAv1E2EParamMinQPAllowTest);
#endif  // LOW_LEVEL_TOOL

/** Test case for high_dynamic_range_input*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamHighDynamicRangeInputTest,
                        high_dynamic_range_input);
PARAM_TEST(SvtAv1E2EParamHighDynamicRangeInputTest);

#if HIGH_LEVEL_TOOL
/** Test case for profile*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamProfileTest, profile);
PARAM_TEST(SvtAv1E2EParamProfileTest);
#endif  // HIGH_LEVEL_TOOL

/** Test case for tier*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTierTest, tier);
PARAM_TEST(SvtAv1E2EParamTierTest);

/** Test case for level*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamLevelTest, level);
PARAM_TEST(SvtAv1E2EParamLevelTest);

/** Test case for asm_type*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamAsmTypeTest, asm_type);
PARAM_TEST(SvtAv1E2EParamAsmTypeTest);

#if MULIT_INSTANCE_TEST_CASE
/** Test case for channel_id*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamChIdTest, channel_id);
PARAM_TEST(SvtAv1E2EParamChIdTest);
/** Test case for active_channel_count*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamActiveChCountTest, active_channel_count);
PARAM_TEST(SvtAv1E2EParamActiveChCountTest);
#endif  // MULIT_INSTANCE_TEST_CASE

/** Test case for speed_control_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSpeedCtrlTest, speed_control_flag);
PARAM_TEST(SvtAv1E2EParamSpeedCtrlTest);

/** Test case for injector_frame_rate*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamInjectorFrameRateTest,
                        injector_frame_rate);
PARAM_TEST(SvtAv1E2EParamInjectorFrameRateTest);

/** Test case for logical_processors*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamLogicalProcessorsTest,
                        logical_processors);
PARAM_TEST(SvtAv1E2EParamLogicalProcessorsTest);

/** Test case for target_socket*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTargetSocketTest, target_socket);
PARAM_TEST(SvtAv1E2EParamTargetSocketTest);

#if TILES
#if HIGH_LEVEL_TOOL
/** Test case for tile_columns*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTileColsTest, tile_columns);
PARAM_TEST(SvtAv1E2EParamTileColsTest);

/** Test case for tile_rows*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTileRowsTest, tile_rows);
PARAM_TEST(SvtAv1E2EParamTileRowsTest);
#endif  // HIGH_LEVEL_TOOL
#endif
