#include "gtest/gtest.h"
extern "C" {
#include "EbAppConfig.h"
#include "EbAppContext.h"
}
#include "EbApi.h"

// TODO(wenyao): This two functions is copied from
// EbAppContext.c & EbAppConfig.c.
/***********************************************
 * Copy configuration parameters from
 *  The config structure, to the
 *  callback structure to send to the library
 ***********************************************/
EbErrorType CopyConfigurationParameters(EbConfig_t *config,
                                        EbAppContext_t *callbackData,
                                        uint32_t instanceIdx) {
    EbErrorType return_error = EB_ErrorNone;
    uint32_t hmeRegionIndex;

    // Assign Instance index to the library
    callbackData->instanceIdx = (uint8_t)instanceIdx;

    // Initialize Port Activity Flags
    callbackData->outputStreamPortActive = APP_PortActive;
    callbackData->ebEncParameters.source_width = config->sourceWidth;
    callbackData->ebEncParameters.source_height = config->sourceHeight;
    callbackData->ebEncParameters.intra_period_length = config->intraPeriod;
    callbackData->ebEncParameters.intra_refresh_type = config->intraRefreshType;
    callbackData->ebEncParameters.base_layer_switch_mode =
        config->base_layer_switch_mode;
    callbackData->ebEncParameters.enc_mode = (EbBool)config->encMode;
    callbackData->ebEncParameters.frame_rate = config->frameRate;
    callbackData->ebEncParameters.frame_rate_denominator =
        config->frameRateDenominator;
    callbackData->ebEncParameters.frame_rate_numerator =
        config->frameRateNumerator;
    callbackData->ebEncParameters.hierarchical_levels =
        config->hierarchicalLevels;
    callbackData->ebEncParameters.pred_structure =
        (uint8_t)config->predStructure;
    callbackData->ebEncParameters.in_loop_me_flag = config->in_loop_me_flag;
    callbackData->ebEncParameters.ext_block_flag = config->ext_block_flag;
#if TILES
    callbackData->ebEncParameters.tile_rows = config->tile_rows;
    callbackData->ebEncParameters.tile_columns = config->tile_columns;
#endif
    callbackData->ebEncParameters.scene_change_detection =
        config->scene_change_detection;
    callbackData->ebEncParameters.look_ahead_distance =
        config->look_ahead_distance;
    callbackData->ebEncParameters.framesToBeEncoded = config->framesToBeEncoded;
    callbackData->ebEncParameters.rate_control_mode = config->rateControlMode;
    callbackData->ebEncParameters.target_bit_rate = config->targetBitRate;
    callbackData->ebEncParameters.max_qp_allowed = config->max_qp_allowed;
    callbackData->ebEncParameters.min_qp_allowed = config->min_qp_allowed;
    callbackData->ebEncParameters.qp = config->qp;
    callbackData->ebEncParameters.use_qp_file = (EbBool)config->use_qp_file;
    callbackData->ebEncParameters.disable_dlf_flag =
        (EbBool)config->disable_dlf_flag;
    callbackData->ebEncParameters.enable_warped_motion =
        (EbBool)config->enable_warped_motion;
    callbackData->ebEncParameters.use_default_me_hme =
        (EbBool)config->use_default_me_hme;
    callbackData->ebEncParameters.enable_hme_flag =
        (EbBool)config->enableHmeFlag;
    callbackData->ebEncParameters.enable_hme_level0_flag =
        (EbBool)config->enableHmeLevel0Flag;
    callbackData->ebEncParameters.enable_hme_level1_flag =
        (EbBool)config->enableHmeLevel1Flag;
    callbackData->ebEncParameters.enable_hme_level2_flag =
        (EbBool)config->enableHmeLevel2Flag;
    callbackData->ebEncParameters.search_area_width = config->searchAreaWidth;
    callbackData->ebEncParameters.search_area_height = config->searchAreaHeight;
    callbackData->ebEncParameters.number_hme_search_region_in_width =
        config->numberHmeSearchRegionInWidth;
    callbackData->ebEncParameters.number_hme_search_region_in_height =
        config->numberHmeSearchRegionInHeight;
    callbackData->ebEncParameters.hme_level0_total_search_area_width =
        config->hmeLevel0TotalSearchAreaWidth;
    callbackData->ebEncParameters.hme_level0_total_search_area_height =
        config->hmeLevel0TotalSearchAreaHeight;
    callbackData->ebEncParameters.constrained_intra =
        (EbBool)config->constrained_intra;
    callbackData->ebEncParameters.channel_id = config->channel_id;
    callbackData->ebEncParameters.active_channel_count =
        config->active_channel_count;
    callbackData->ebEncParameters.improve_sharpness =
        (uint8_t)config->improve_sharpness;
    callbackData->ebEncParameters.high_dynamic_range_input =
        config->high_dynamic_range_input;
    callbackData->ebEncParameters.access_unit_delimiter =
        config->access_unit_delimiter;
    callbackData->ebEncParameters.buffering_period_sei =
        config->buffering_period_sei;
    callbackData->ebEncParameters.picture_timing_sei =
        config->picture_timing_sei;
    callbackData->ebEncParameters.registered_user_data_sei_flag =
        config->registered_user_data_sei_flag;
    callbackData->ebEncParameters.unregistered_user_data_sei_flag =
        config->unregistered_user_data_sei_flag;
    callbackData->ebEncParameters.recovery_point_sei_flag =
        config->recovery_point_sei_flag;
    callbackData->ebEncParameters.enable_temporal_id =
        config->enable_temporal_id;
    callbackData->ebEncParameters.encoder_bit_depth = config->encoderBitDepth;
    callbackData->ebEncParameters.compressed_ten_bit_format =
        config->compressedTenBitFormat;
    callbackData->ebEncParameters.profile = config->profile;
    callbackData->ebEncParameters.tier = config->tier;
    callbackData->ebEncParameters.level = config->level;
    callbackData->ebEncParameters.injector_frame_rate =
        config->injector_frame_rate;
    callbackData->ebEncParameters.speed_control_flag =
        config->speed_control_flag;
    callbackData->ebEncParameters.asm_type = config->asmType;
    callbackData->ebEncParameters.logical_processors =
        config->logicalProcessors;
    callbackData->ebEncParameters.target_socket = config->targetSocket;
    callbackData->ebEncParameters.recon_enabled =
        config->reconFile ? EB_TRUE : EB_FALSE;

    for (hmeRegionIndex = 0;
         hmeRegionIndex <
         callbackData->ebEncParameters.number_hme_search_region_in_width;
         ++hmeRegionIndex) {
        callbackData->ebEncParameters
            .hme_level0_search_area_in_width_array[hmeRegionIndex] =
            config->hmeLevel0SearchAreaInWidthArray[hmeRegionIndex];
        callbackData->ebEncParameters
            .hme_level1_search_area_in_width_array[hmeRegionIndex] =
            config->hmeLevel1SearchAreaInWidthArray[hmeRegionIndex];
        callbackData->ebEncParameters
            .hme_level2_search_area_in_width_array[hmeRegionIndex] =
            config->hmeLevel2SearchAreaInWidthArray[hmeRegionIndex];
    }

    for (hmeRegionIndex = 0;
         hmeRegionIndex <
         callbackData->ebEncParameters.number_hme_search_region_in_height;
         ++hmeRegionIndex) {
        callbackData->ebEncParameters
            .hme_level0_search_area_in_height_array[hmeRegionIndex] =
            config->hmeLevel0SearchAreaInHeightArray[hmeRegionIndex];
        callbackData->ebEncParameters
            .hme_level1_search_area_in_height_array[hmeRegionIndex] =
            config->hmeLevel1SearchAreaInHeightArray[hmeRegionIndex];
        callbackData->ebEncParameters
            .hme_level2_search_area_in_height_array[hmeRegionIndex] =
            config->hmeLevel2SearchAreaInHeightArray[hmeRegionIndex];
    }

    return return_error;
}

void EbConfigCtor(EbConfig_t *config_ptr) {
    config_ptr->configFile = NULL;
    config_ptr->inputFile = NULL;
    config_ptr->bitstreamFile = NULL;
    config_ptr->reconFile = NULL;
    config_ptr->errorLogFile = stderr;
    config_ptr->qpFile = NULL;

    config_ptr->frameRate = 30 << 16;
    config_ptr->frameRateNumerator = 0;
    config_ptr->frameRateDenominator = 0;
    config_ptr->encoderBitDepth = 8;
    config_ptr->compressedTenBitFormat = 0;
    config_ptr->sourceWidth = 0;
    config_ptr->sourceHeight = 0;
    config_ptr->inputPaddedWidth = 0;
    config_ptr->inputPaddedHeight = 0;
    config_ptr->framesToBeEncoded = 0;
    config_ptr->bufferedInput = -1;
    config_ptr->sequenceBuffer = 0;
    config_ptr->latencyMode = 0;

    // Interlaced Video
    config_ptr->interlacedVideo = EB_FALSE;
    config_ptr->separateFields = EB_FALSE;
    config_ptr->qp = 50;
    config_ptr->use_qp_file = EB_FALSE;

    config_ptr->scene_change_detection = 0;
    config_ptr->rateControlMode = 0;
    config_ptr->look_ahead_distance = (uint32_t)~0;
    config_ptr->targetBitRate = 7000000;
    config_ptr->max_qp_allowed = 63;
    config_ptr->min_qp_allowed = 0;
    config_ptr->base_layer_switch_mode = 0;
    config_ptr->encMode = MAX_ENC_PRESET;
    config_ptr->intraPeriod = -2;
    config_ptr->intraRefreshType = 1;
    config_ptr->hierarchicalLevels = 4;
    config_ptr->predStructure = 2;
    config_ptr->disable_dlf_flag = EB_FALSE;
    config_ptr->enable_warped_motion = EB_FALSE;
    config_ptr->ext_block_flag = EB_FALSE;
    config_ptr->in_loop_me_flag = EB_TRUE;
    config_ptr->use_default_me_hme = EB_TRUE;
    config_ptr->enableHmeFlag = EB_TRUE;
    config_ptr->enableHmeLevel0Flag = EB_TRUE;
    config_ptr->enableHmeLevel1Flag = EB_FALSE;
    config_ptr->enableHmeLevel2Flag = EB_FALSE;
    config_ptr->searchAreaWidth = 16;
    config_ptr->searchAreaHeight = 7;
    config_ptr->numberHmeSearchRegionInWidth = 2;
    config_ptr->numberHmeSearchRegionInHeight = 2;
    config_ptr->hmeLevel0TotalSearchAreaWidth = 64;
    config_ptr->hmeLevel0TotalSearchAreaHeight = 25;
    config_ptr->hmeLevel0ColumnIndex = 0;
    config_ptr->hmeLevel0RowIndex = 0;
    config_ptr->hmeLevel1ColumnIndex = 0;
    config_ptr->hmeLevel1RowIndex = 0;
    config_ptr->hmeLevel2ColumnIndex = 0;
    config_ptr->hmeLevel2RowIndex = 0;
    config_ptr->hmeLevel0SearchAreaInWidthArray[0] = 32;
    config_ptr->hmeLevel0SearchAreaInWidthArray[1] = 32;
    config_ptr->hmeLevel0SearchAreaInHeightArray[0] = 12;
    config_ptr->hmeLevel0SearchAreaInHeightArray[1] = 13;
    config_ptr->hmeLevel1SearchAreaInWidthArray[0] = 1;
    config_ptr->hmeLevel1SearchAreaInWidthArray[1] = 1;
    config_ptr->hmeLevel1SearchAreaInHeightArray[0] = 1;
    config_ptr->hmeLevel1SearchAreaInHeightArray[1] = 1;
    config_ptr->hmeLevel2SearchAreaInWidthArray[0] = 1;
    config_ptr->hmeLevel2SearchAreaInWidthArray[1] = 1;
    config_ptr->hmeLevel2SearchAreaInHeightArray[0] = 1;
    config_ptr->hmeLevel2SearchAreaInHeightArray[1] = 1;
    config_ptr->constrained_intra = 0;
    config_ptr->film_grain_denoise_strength = 0;

    // Thresholds
    config_ptr->high_dynamic_range_input = 0;
    config_ptr->access_unit_delimiter = 0;
    config_ptr->buffering_period_sei = 0;
    config_ptr->picture_timing_sei = 0;

    config_ptr->improve_sharpness = 0;
    config_ptr->registered_user_data_sei_flag = 0;
    config_ptr->unregistered_user_data_sei_flag = 0;
    config_ptr->recovery_point_sei_flag = 0;
    config_ptr->enable_temporal_id = 1;

    // Annex A parameters
    config_ptr->profile = 0;
    config_ptr->tier = 0;
    config_ptr->level = 0;

    // Latency
    config_ptr->injector = 0;
    config_ptr->injector_frame_rate = 60 << 16;
    config_ptr->speed_control_flag = 0;

    // Testing
    config_ptr->testUserData = 0;
    config_ptr->eosFlag = 0;

    // Computational Performance Parameters
    config_ptr->performanceContext.lib_start_time[0] = 0;
    config_ptr->performanceContext.lib_start_time[1] = 0;

    config_ptr->performanceContext.encode_start_time[0] = 0;
    config_ptr->performanceContext.encode_start_time[1] = 0;

    config_ptr->performanceContext.total_execution_time = 0;
    config_ptr->performanceContext.total_encode_time = 0;

    config_ptr->performanceContext.frameCount = 0;
    config_ptr->performanceContext.averageSpeed = 0;
    config_ptr->performanceContext.startsTime = 0;
    config_ptr->performanceContext.startuTime = 0;
    config_ptr->performanceContext.maxLatency = 0;
    config_ptr->performanceContext.totalLatency = 0;
    config_ptr->performanceContext.byteCount = 0;

    // ASM Type
    config_ptr->asmType = 1;

    config_ptr->stopEncoder = 0;
    config_ptr->logicalProcessors = 0;
    config_ptr->targetSocket = -1;
    config_ptr->processedFrameCount = 0;
    config_ptr->processedByteCount = 0;
#if TILES
    config_ptr->tile_rows = 0;
    config_ptr->tile_columns = 0;
#endif
    config_ptr->byte_count_since_ivf = 0;
    config_ptr->ivf_count = 0;
    return;
}

TEST(EncoderAPI, NullPointParam) {
    // testing::internal::CaptureStdout();
    EbAppContext_t appCallback = {0};
    // Test null pointer, expect BadParameters
    EXPECT_EQ(EB_ErrorBadParameter, eb_init_handle(nullptr, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter,
              eb_init_handle(&appCallback.svtEncoderHandle, nullptr, nullptr));
    EXPECT_EQ(EB_ErrorBadParameter, eb_svt_enc_set_parameter(nullptr, nullptr));
    // TODO(Ryan): Some function will crash with nullptr input,
    // and it will block test on linux platform.
    // EXPECT_EQ(EB_ErrorBadParameter,
    //          eb_svt_enc_set_parameter(appCallback.svtEncoderHandle,
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