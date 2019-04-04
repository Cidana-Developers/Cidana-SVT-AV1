/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"

using namespace svt_av1_test_e2e;
using namespace svt_av1_e2e_test_vector;

class SvtAv1E2ESimpleTest : public SvtAv1E2ETestFramework {
protected:
	void init_test_output_file(std::string path){
		output_file_ = new IvfFile(path);
		SvtAv1E2ETestFramework::init_test();
	}
};

TEST_P(SvtAv1E2ESimpleTest, run_smoking_test) {
    init_test();
    run_encode_process();
    close_test();
}

TEST_P(SvtAv1E2ESimpleTest, run_smoking_with_output_test) {
	init_test_output_file("output.av1");
	run_encode_process();
	close_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ESimpleTest,
                        ::testing::ValuesIn(smoking_vectors));

class SvtAv1E2EReconFileTest : public SvtAv1E2ETestFramework {
  protected:
    virtual void init_test() final override {
        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_sink_ = create_recon_sink(param, "enc_recon.rcs");
        ASSERT_NE(recon_sink_, nullptr) << "can not create recon sink!!";
        if (recon_sink_)
            ctxt_.enc_params.recon_enabled = 1;
        SvtAv1E2ETestFramework::init_test();
    }
};

TEST_P(SvtAv1E2EReconFileTest, run_recon_collect_test) {
	init_test();
	run_encode_process();
	close_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EReconFileTest,
	::testing::ValuesIn(recon_file_vectors));

class SvtAv1E2EReconBufferTest : public SvtAv1E2ETestFramework {
protected:
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
		SvtAv1E2ETestFramework::init_test();
	}
};

TEST_P(SvtAv1E2EReconBufferTest, run_recon_collect_test) {
	init_test();
	run_encode_process();
	close_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EReconBufferTest,
	::testing::ValuesIn(recon_file_vectors));


class SvtAv1E2EConformanceTest : public SvtAv1E2ETestFramework {
protected:
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
};

TEST_P(SvtAv1E2EConformanceTest, DISABLE_run_conformance_test) {
	init_test();
	run_encode_process();
	close_test();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EConformanceTest,
	::testing::ValuesIn(recon_file_vectors));
