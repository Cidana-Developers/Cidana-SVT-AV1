/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"
#include "../api_test/params.h"

using namespace svt_av1_test_e2e;
using namespace svt_av1_e2e_test_vector;

class SvtAv1E2EParamBase : public SvtAv1E2ETestFramework {
  protected:
    SvtAv1E2EParamBase(std::string param_name) {
        param_name_str_ = param_name;
    }
    virtual ~SvtAv1E2EParamBase() {
    }
    virtual void init_test() final override {
        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_sink_ = create_recon_sink(param);
        ASSERT_NE(recon_sink_, nullptr) << "can not create recon sink!!";
        if (recon_sink_)
            ctxt_.enc_params.recon_enabled = 1;

        // create reference decoder
        refer_dec_ = create_reference_decoder();
        ASSERT_NE(refer_dec_, nullptr) << "can not create reference decoder!!";

        SvtAv1E2ETestFramework::init_test();
    }

  protected:
    std::string param_name_str_; /**< name of parameter for test */
};

/** Marcro defininition of batch processing check for default, valid, invalid
 * and special parameter check*/
#define PARAM_TEST(param_test)                          \
    TEST_P(param_test, run_paramter_conformance_test) { \
        run_conformance_test();                         \
    }                                                   \
    INSTANTIATE_TEST_CASE_P(                            \
        SVT_AV1, param_test, ::testing::ValuesIn(smoking_vectors));

/** @breif This class is a template based on EncParamTestBase to test each
 * parameter
 */
#define DEFINE_PARAM_TEST_CLASS(test_name, param_name)                    \
    class test_name : public SvtAv1E2EParamBase {                         \
      public:                                                             \
        test_name() : SvtAv1E2EParamBase(#param_name) {                   \
        }                                                                 \
        virtual void init_test(size_t i) {                                \
            ctxt_.enc_params.param_name = GET_VALID_PARAM(param_name, i); \
            SvtAv1E2EParamBase::init_test();                              \
        }                                                                 \
        virtual void run_conformance_test() {                    \
            for (size_t i = 0; i < SIZE_VALID_PARAM(param_name); ++i) {   \
                SvtAv1E2EParamBase::SetUp();                              \
                init_test(i);                                             \
                run_encode_process();                                     \
                close_test();                                             \
                SvtAv1E2EParamBase::TearDown();                           \
            }                                                             \
        }                                                                 \
                                                                          \
      protected:                                                          \
        virtual void SetUp() override {                                   \
            /* skip SvtAv1E2EParamBase::SetUp() */                        \
        }                                                                 \
        virtual void TearDown() override {                                \
            /* skip SvtAv1E2EParamBase::TearDown() */                     \
        }                                                                 \
    };

/** Test case for enc_mode*/
DEFINE_PARAM_TEST_CLASS(EncE2EParamEncModeTest, enc_mode);
PARAM_TEST(EncE2EParamEncModeTest);
