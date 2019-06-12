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

#include <map>
#include <cmath>
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"
#include "../api_test/params.h"

/** low/high level tools are not ready, following macros mark to hide
 * unsupported test cases, will be removed after full-covered */
#define NOT_SUPPORTED 0

#define THIS_TEST_IS_DEATH 1

/**
 * @brief SVT-AV1 encoder parameter coverage E2E test
 *
 * Test strategy:
 * Config SVT-AV1 encoder with individual parameter, run the
 * conformance test and analyse the bitstream to check if the params
 * take effect.
 *
 * Expected result:
 * No error is reported in encoding progress. The reconstructed frame
 * data is same as the output frame from reference decoder.
 *
 * Test coverage:
 * Almost all the encoder parameters except frame_rate_numerator and
 * frame_rate_denominator.
 */

using namespace svt_av1_e2e_test;
using namespace svt_av1_e2e_test_vector;

/** Max/Min QP
 *	function CopyApiFromApp() in EbEncHandle.c transfers "min_qp_allowed" to
 *  1 when in non-rate-control mode, test vectors of
 *  qp/min_qp_allowed/max_qp_allowed should stop test when value equals
 *  MIN_QP_VALUE
 *
 * code piece:
 *	 sequence_control_set_ptr->static_config.max_qp_allowed
 *=(sequence_control_set_ptr->static_config.rate_control_mode) ?
 *((EbSvtAv1EncConfiguration*)pComponentParameterStructure)->max_qp_allowed: 63;
 *
 *	sequence_control_set_ptr->static_config.min_qp_allowed
 *=(sequence_control_set_ptr->static_config.rate_control_mode) ?
 *((EbSvtAv1EncConfiguration*)pComponentParameterStructure)->min_qp_allowed: 1;
 */
static const std::multimap<const std::string, const std::string>
    IGNORE_CASE_MAP = {
        {"intra_period_length", "-2"}, /**< -2 means auto */
        {"qp", "0"},
        {"max_qp_allowed", "0"},
        {"min_qp_allowed", "0"},
        {"use_qp_file",
         "0"}, /**< 0 means not use qp file, test is unnecessary*/
        {"target_bit_rate", "0"},
        {"target_bit_rate", "1"},
        {"target_bit_rate", "100"},
        {"target_bit_rate",
         "1000"}, /**<^ bit-rate is too small for test vector */
};

class SvtAv1E2EParamFramework : public SvtAv1E2ETestFramework {
  public:
    SvtAv1E2EParamFramework() {
        param_value_idx_ = 0;
        skip_vector_ = false;
        enable_analyzer_ = false;
    }

    virtual ~SvtAv1E2EParamFramework() {
    }

    /** customize encoder param and setup conformance test */
    void init_test() override {
        skip_vector_ = false;
        /** customize the encoder param */
        config_enc_param();

        if (!skip_vector_) {
            /** create recon frame queue before setup parameter of encoder */
            VideoFrameParam param;
            memset(&param, 0, sizeof(param));
            param.format = video_src_->get_image_format();
            param.width = video_src_->get_width_with_padding();
            param.height = video_src_->get_height_with_padding();
            recon_queue_ = create_frame_queue(param);
            ASSERT_NE(recon_queue_, nullptr) << "can not create recon queue!!";
            if (recon_queue_)
                av1enc_ctx_.enc_params.recon_enabled = 1;

            /** create reference decoder*/
            refer_dec_ = create_reference_decoder(enable_analyzer_);
            ASSERT_NE(refer_dec_, nullptr)
                << "can not create reference decoder!!";
            refer_dec_->set_resolution(video_src_->get_width_with_padding(),
                                       video_src_->get_height_with_padding());

            SvtAv1E2ETestFramework::init_test();
        }
    }

    /** check the bitstream if the enc param take effects */
    void close_test() override {
        verify_enc_param();
        SvtAv1E2ETestFramework::close_test();
    }

  protected:
    /** setup some of the params with related params modified before set
     * to encoder */
    void config_enc_param() {
        if (!param_name_str_.compare("film_grain_denoise_strength")) {
            av1enc_ctx_.enc_params.enable_denoise_flag = 1;
        } else if (!param_name_str_.compare("target_bit_rate")) {
            av1enc_ctx_.enc_params.rate_control_mode = 1;
            /** fixed frame-rate for calculation */
            if (av1enc_ctx_.enc_params.frame_rate > 1000)
                av1enc_ctx_.enc_params.frame_rate = 10 << 16;
            else
                av1enc_ctx_.enc_params.frame_rate = 10;
        } else if (!param_name_str_.compare("injector_frame_rate")) {
            av1enc_ctx_.enc_params.speed_control_flag = 1;
        } else if (!param_name_str_.compare("profile")) {
            if (av1enc_ctx_.enc_params.profile == 0) {
                /** profile(0) requires YUV420 */
                if (av1enc_ctx_.enc_params.encoder_color_format != EB_YUV420) {
                    skip_vector_ = true;
                }
            } else if (av1enc_ctx_.enc_params.profile == 1) {
                /** profile(1) requires 8-bit YUV444 */
                if (av1enc_ctx_.enc_params.encoder_bit_depth != 8 ||
                    av1enc_ctx_.enc_params.encoder_color_format != EB_YUV444) {
                    skip_vector_ = true;
                }
            } else if (av1enc_ctx_.enc_params.profile == 2) {
                /** profile(2) requires 8-bit/10-bit YUV422 */
                if (av1enc_ctx_.enc_params.encoder_bit_depth < 8 ||
                    av1enc_ctx_.enc_params.encoder_bit_depth > 10 ||
                    av1enc_ctx_.enc_params.encoder_color_format != EB_YUV422) {
                    skip_vector_ = true;
                }
            }
        } else if (!param_name_str_.compare("tile_columns")) {
            /** update tile setting to actual tile columns */
            uint32_t value = std::stoul(param_value_str_);
            uint32_t cols = 0;
            if (value == 0) {  // no tiling
                cols = video_src_->get_width_with_padding() / 4;
            } else {  // tiling
                cols = (video_src_->get_width_with_padding() / 4) /
                       std::pow(2, value);
                cols = cols < 16 ? 16 : ((cols + 15) >> 4) << 4;
            }
            param_value_str_ = std::to_string(cols);
        } else if (!param_name_str_.compare("tile_rows")) {
            /** update tile setting to actual tile rows */
            uint32_t value = std::stoul(param_value_str_);
            uint32_t rows = 0;
            if (value == 0) {  // no tiling
                rows = video_src_->get_height_with_padding() / 4;
            } else {  // tiling
                rows = (video_src_->get_height_with_padding() / 4) /
                       std::pow(2, value);
                rows = rows < 16 ? 16 : ((rows + 15) >> 4) << 4;
            }
            param_value_str_ = std::to_string(rows);
        } else if (!param_name_str_.compare("qp")) {
            av1enc_ctx_.enc_params.rate_control_mode = 0;
            av1enc_ctx_.enc_params.min_qp_allowed = MIN_QP_VALUE;
            av1enc_ctx_.enc_params.max_qp_allowed = MAX_QP_VALUE;
        } else if (!param_name_str_.compare("enable_qp_scaling_flag")) {
            av1enc_ctx_.enc_params.rate_control_mode = 0;
            av1enc_ctx_.enc_params.min_qp_allowed = MIN_QP_VALUE;
            av1enc_ctx_.enc_params.max_qp_allowed = MAX_QP_VALUE;
        } else if (!param_name_str_.compare("max_qp_allowed")) {
            uint32_t value = std::stoul(param_value_str_);
            av1enc_ctx_.enc_params.rate_control_mode = 0;
            av1enc_ctx_.enc_params.min_qp_allowed = MIN_QP_VALUE;
            if (av1enc_ctx_.enc_params.qp > value)
                av1enc_ctx_.enc_params.qp = value;
        } else if (!param_name_str_.compare("min_qp_allowed")) {
            uint32_t value = std::stoul(param_value_str_);
            av1enc_ctx_.enc_params.rate_control_mode = 0;
            av1enc_ctx_.enc_params.max_qp_allowed = MAX_QP_VALUE;
            if (av1enc_ctx_.enc_params.qp < value)
                av1enc_ctx_.enc_params.qp = value;
        } else if (!param_name_str_.compare("use_qp_file")) {
            use_ext_qp_ = true;
        }
    }

    /** additional process after parameter test finish */
    void verify_enc_param() {
        if (enable_analyzer_ && refer_dec_ && !is_ignored()) {
            std::string result =
                refer_dec_->get_syntax_element(param_name_str_);
            if (!param_name_str_.compare("max_qp_allowed")) {
                uint32_t value = std::stoul(param_value_str_);
                uint32_t res = std::stoul(result);
                ASSERT_GE(value, res)
                    << "e2e parameter " << param_name_str_ << " test failed";
            } else if (!param_name_str_.compare("min_qp_allowed")) {
                uint32_t value = std::stoul(param_value_str_);
                uint32_t res = std::stoul(result);
                ASSERT_LE(value, res)
                    << "e2e parameter " << param_name_str_ << " test failed";
            } else if (!param_name_str_.compare("use_qp_file")) {
                uint32_t frame_count = video_src_->get_frame_count();
                for (uint32_t i = 1; i < frame_count; i++) {
                    uint32_t res = std::stoul(
                        refer_dec_->get_syntax_element(param_name_str_, i - 1));
                    EXPECT_EQ(video_src_->get_frame_qp(i), res)
                        << "e2e parameter " << param_name_str_
                        << " test failed at " << i;
                }
            } else if (!param_name_str_.compare("target_bit_rate")) {
                uint32_t value = std::stoul(param_value_str_);
                uint32_t res = std::stoul(result);
                uint32_t frame_rate =
                    av1enc_ctx_.enc_params.frame_rate > 1000
                        ? av1enc_ctx_.enc_params.frame_rate >> 16
                        : av1enc_ctx_.enc_params.frame_rate;
                ASSERT_GE(value, res * frame_rate)
                    << "e2e parameter " << param_name_str_ << " test failed";
            } else {
                ASSERT_STREQ(param_value_str_.c_str(), result.c_str())
                    << "e2e parameter " << param_name_str_ << " test failed";
            }
        }
    }

    /** check whether the pair of param name and param value in ignore list */
    bool is_ignored() {
        typedef std::map<const std::string, const std::string>::const_iterator
            c_it;
        std::pair<c_it, c_it> range =
            IGNORE_CASE_MAP.equal_range(param_name_str_);
        for (c_it it = range.first; it != range.second; ++it) {
            if (!it->second.compare(param_value_str_))
                return true;
        }
        return false;
    }

  protected:
    std::string param_name_str_;  /**< name of parameter for test */
    std::string param_value_str_; /**< value of parameter for test */
    size_t param_value_idx_;      /**< index of parameter value in vector */
    bool skip_vector_;            /**< flag of skip this test vector or not */
    bool enable_analyzer_;        /**< flag of with or without parser */
};

/** Marcro defininition of batch processing check for default, valid, invalid
 * and special parameter check*/
#define PARAM_TEST_WITH_VECTOR(param_test, vectors)     \
    TEST_P(param_test, run_paramter_conformance_test) { \
        run_conformance_test();                         \
    }                                                   \
    INSTANTIATE_TEST_CASE_P(SVT_AV1, param_test, ::testing::ValuesIn(vectors));

#define PARAM_TEST(param_test) \
    PARAM_TEST_WITH_VECTOR(param_test, smoking_vectors)

#define PARAM_DEATHTEST_WITH_VECTOR(param_test, vectors) \
    TEST_P(param_test, run_paramter_conformance_test) {  \
        run_conformance_death_test();                    \
    }                                                    \
    INSTANTIATE_TEST_CASE_P(SVT_AV1, param_test, ::testing::ValuesIn(vectors));

#define PARAM_DEATHTEST(param_test) \
    PARAM_DEATHTEST_WITH_VECTOR(param_test, smoking_vectors)

#define GET_PARAM GET_VALID_PARAM
#define SIZE_PARAM SIZE_VALID_PARAM

/** @breif This class is a template based on EncParamTestBase to test each
 * parameter
 */
#define DEFINE_PARAM_TEST_CLASS_WT_PARSER(test_name, param_name, wt_parser)    \
    class test_name : public SvtAv1E2EParamFramework {                         \
      public:                                                                  \
        test_name() {                                                          \
            param_name_str_ = #param_name;                                     \
            enable_analyzer_ = wt_parser;                                      \
        }                                                                      \
        /** setup target parameter name */                                     \
        void init_test() override {                                            \
            collect_ = new PerformanceCollect(typeid(this).name());            \
            EXPECT_NE(collect_, nullptr) << "performance tool create failed!"; \
            av1enc_ctx_.enc_params.param_name =                                \
                GET_PARAM(param_name, param_value_idx_);                       \
            param_value_str_ =                                                 \
                std::to_string(av1enc_ctx_.enc_params.param_name);             \
            SvtAv1E2EParamFramework::init_test();                              \
        }                                                                      \
        /** close for test */                                                  \
        void close_test() override {                                           \
            SvtAv1E2EParamFramework::close_test();                             \
            if (collect_) {                                                    \
                delete collect_;                                               \
                collect_ = nullptr;                                            \
            }                                                                  \
        }                                                                      \
        /** run for the conformance test */                                    \
        void run_conformance_test() {                                          \
            for (param_value_idx_ = 0;                                         \
                 param_value_idx_ < SIZE_PARAM(param_name);                    \
                 ++param_value_idx_) {                                         \
                SvtAv1E2EParamFramework::SetUp();                              \
                if (!skip_vector_)                                             \
                    run_encode_process();                                      \
                else {                                                         \
                    printf("test %s(%s) skipped vector\n",                     \
                           param_name_str_.c_str(),                            \
                           param_value_str_.c_str());                          \
                }                                                              \
                SvtAv1E2EParamFramework::TearDown();                           \
            }                                                                  \
        }                                                                      \
        /** run for the conformance death test */                              \
        void run_conformance_death_test() {                                    \
            testing::FLAGS_gtest_death_test_style = "threadsafe";              \
            for (param_value_idx_ = 0;                                         \
                 param_value_idx_ < SIZE_PARAM(param_name);                    \
                 ++param_value_idx_) {                                         \
                SvtAv1E2EParamFramework::SetUp();                              \
                if (!skip_vector_)                                             \
                    EXPECT_DEATH(run_encode_process(), "");                    \
                else {                                                         \
                    printf("test %s(%s) skipped vector\n",                     \
                           param_name_str_.c_str(),                            \
                           param_value_str_.c_str());                          \
                }                                                              \
                SvtAv1E2EParamFramework::TearDown();                           \
            }                                                                  \
        }                                                                      \
                                                                               \
      protected:                                                               \
        void SetUp() override {                                                \
            /* skip SvtAv1E2ETestFramework::SetUp() */                         \
        }                                                                      \
        void TearDown() override {                                             \
            /* skip SvtAv1E2ETestFramework::TearDown() */                      \
        }                                                                      \
    };

/** class definiion for parameter test without parser */
#define DEFINE_PARAM_TEST_CLASS(test_name, param_name) \
    DEFINE_PARAM_TEST_CLASS_WT_PARSER(test_name, param_name, false)

/** class definiion for parameter test with parser */
#define DEFINE_PARAM_TEST_CLASS_EX(test_name, param_name) \
    DEFINE_PARAM_TEST_CLASS_WT_PARSER(test_name, param_name, true)

/** Test case for enc_mode*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEncModeTest, enc_mode);
PARAM_TEST(SvtAv1E2EParamEncModeTest);

/** Test case for intra_period_length*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamIntraPeridLenTest,
                           intra_period_length);
PARAM_TEST(SvtAv1E2EParamIntraPeridLenTest);

#if NOT_SUPPORTED
/** Test case for intra_refresh_type*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamIntraRefreshTypeTest, intra_refresh_type);
PARAM_TEST(SvtAv1E2EParamIntraRefreshTypeTest);

/** Test case for hierarchical_levels*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamHierarchicalLvlTest, hierarchical_levels);
PARAM_TEST(SvtAv1E2EParamHierarchicalLvlTest);

/** Test case for pred_structure*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamPredStructTest, pred_structure);
PARAM_TEST(SvtAv1E2EParamPredStructTest);

/** Test case for base_layer_switch_mode*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamBaseLayerSwitchModeTest,
                        pred_stbase_layer_switch_moderucture);
PARAM_TEST(SvtAv1E2EParamBaseLayerSwitchModeTest);
#endif  // NOT_SUPPORTED

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

#if NOT_SUPPORTED
/** Test case for sb_sz*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSbSizeTest, sb_sz);
PARAM_TEST(SvtAv1E2EParamSbSizeTest);
#endif  // NOT_SUPPORTED

/** Test case for super_block_size*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamSuperBlockSizeTest, super_block_size);
PARAM_TEST(SvtAv1E2EParamSuperBlockSizeTest);

/** Test case for partition_depth*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamPartitionDepthTest, partition_depth);
PARAM_TEST(SvtAv1E2EParamPartitionDepthTest);

/** Test case for qp*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamQPTest, qp);
PARAM_TEST(SvtAv1E2EParamQPTest);

/** Test case for use_qp_file*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamUseQPFileTest, use_qp_file);
PARAM_TEST(SvtAv1E2EParamUseQPFileTest);

/** Test case for enable_qp_scaling_flag*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamEnableQPScaleTest,
                           enable_qp_scaling_flag);
PARAM_TEST(SvtAv1E2EParamEnableQPScaleTest);

#if NOT_SUPPORTED
/** Test case for disable_dlf_flag*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamDisableDlfTest, disable_dlf_flag);
PARAM_TEST(SvtAv1E2EParamDisableDlfTest);
#endif  // NOT_SUPPORTED

/** Test case for enable_denoise_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableDenoiseTest, enable_denoise_flag);
PARAM_TEST(SvtAv1E2EParamEnableDenoiseTest);

#if THIS_TEST_IS_DEATH
/** Test case for film_grain_denoise_strength*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamFilmGrainDenoiseStrTest,
                        film_grain_denoise_strength);
#if THIS_TEST_VECTOR_CRASHES
PARAM_TEST_WITH_VECTOR(SvtAv1E2EParamFilmGrainDenoiseStrTest,
                       comformance_test_vectors);
#else
PARAM_TEST(SvtAv1E2EParamFilmGrainDenoiseStrTest);
#endif  // THIS_TEST_VECTOR_CRASHES
#endif  // THIS_TEST_IS_DEATH

#if NOT_SUPPORTED
/** Test case for enable_warped_motion*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamEnableWarpedMotionTest,
                           enable_warped_motion);
PARAM_TEST(SvtAv1E2EParamEnableWarpedMotionTest);
#endif  // NOT_SUPPORTED

/** Test case for use_default_me_hme*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamUseDefaultMeHmeTest, use_default_me_hme);
PARAM_TEST(SvtAv1E2EParamUseDefaultMeHmeTest);

/** Test case for enable_hme_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamEnableHmeTest, enable_hme_flag);
PARAM_TEST(SvtAv1E2EParamEnableHmeTest);

/** Test case for ext_block_flag*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamExtBlockTest, ext_block_flag);
PARAM_TEST(SvtAv1E2EParamExtBlockTest);

/** Test case for in_loop_me_flag*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamInLoopMeTest, in_loop_me_flag);
PARAM_TEST(SvtAv1E2EParamInLoopMeTest);

/** Test case for search_area_width*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSearchAreaWidthTest, search_area_width);
PARAM_TEST(SvtAv1E2EParamSearchAreaWidthTest);

/** Test case for search_area_height*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSearchAreaHeightTest, search_area_height);
PARAM_TEST(SvtAv1E2EParamSearchAreaHeightTest);

#if NOT_SUPPORTED
/** Test case for constrained_intra*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamConstrainedIntraTest, constrained_intra);
PARAM_TEST(SvtAv1E2EParamConstrainedIntraTest);
#endif  // NOT_SUPPORTED

/** Test case for rate_control_mode*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamRateCtrlModeTest, rate_control_mode);
PARAM_TEST(SvtAv1E2EParamRateCtrlModeTest);

#if NOT_SUPPORTED
/** Test case for scene_change_detection*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamSceneChangeDectTest,
                        scene_change_detection);
PARAM_TEST(SvtAv1E2EParamSceneChangeDectTest);

/** Test case for look_ahead_distance*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamLookAheadDistanceTest,
                        look_ahead_distance);
PARAM_TEST(SvtAv1E2EParamLookAheadDistanceTest);
#endif  // NOT_SUPPORTED

/** Test case for target_bit_rate*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamTargetBitRateTest, target_bit_rate);
PARAM_TEST(SvtAv1E2EParamTargetBitRateTest);

/** Test case for max_qp_allowed*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamMaxQPAllowTest, max_qp_allowed);
PARAM_TEST(SvtAv1E2EParamMaxQPAllowTest);

/** Test case for min_qp_allowed*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamMinQPAllowTest, min_qp_allowed);
PARAM_TEST(SvtAv1E2EParamMinQPAllowTest);

#if NOT_SUPPORTED
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamHighDynamicRangeInputTest,
                        high_dynamic_range_input);
PARAM_TEST(SvtAv1E2EParamHighDynamicRangeInputTest);
#endif  // NOT_SUPPORTED

/** Test case for profile*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamProfileTest, profile);
PARAM_TEST_WITH_VECTOR(SvtAv1E2EParamProfileTest, profile_vectors);

/** Test case for tier*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamTierTest, tier);
PARAM_TEST(SvtAv1E2EParamTierTest);

/** Test case for level*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamLevelTest, level);
PARAM_TEST(SvtAv1E2EParamLevelTest);

#if THIS_TEST_IS_DEATH
#undef GET_PARAM
#undef SIZE_PARAM
#define GET_PARAM GET_DEATH_PARAM
#define SIZE_PARAM SIZE_DEATH_PARAM
/** Test case for asm_type*/
DEFINE_PARAM_TEST_CLASS(SvtAv1E2EParamAsmTypeTest, asm_type);
PARAM_DEATHTEST(SvtAv1E2EParamAsmTypeTest);
#undef GET_PARAM
#undef SIZE_PARAM
#define GET_PARAM GET_VALID_PARAM
#define SIZE_PARAM SIZE_VALID_PARAM
#endif

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
/** Test case for tile_columns*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamTileColsTest, tile_columns);
PARAM_TEST(SvtAv1E2EParamTileColsTest);

/** Test case for tile_rows*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamTileRowsTest, tile_rows);
PARAM_TEST(SvtAv1E2EParamTileRowsTest);
#endif

/** Following are some special test cases for death test issue*/

#if THIS_TEST_IS_DEATH
#undef GET_PARAM
#undef SIZE_PARAM
#define GET_PARAM GET_DEATH_PARAM
#define SIZE_PARAM SIZE_DEATH_PARAM
/** Death test for QP (63 with single frame), link to issue #263*/
DEFINE_PARAM_TEST_CLASS_EX(SvtAv1E2EParamQPDeathTest, qp);
PARAM_TEST_WITH_VECTOR(SvtAv1E2EParamQPDeathTest, qp_death_test_vectors);
#undef GET_PARAM
#undef SIZE_PARAM
#define GET_PARAM GET_VALID_PARAM
#define SIZE_PARAM SIZE_VALID_PARAM
#endif
