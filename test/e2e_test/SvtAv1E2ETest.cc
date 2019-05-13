/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file SvtAv1E2ETest.cc
 *
 * @brief Impelmentation of SVT-AV1 encoder E2E test
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"
#include "SvtAv1E2EFramework.h"

using namespace svt_av1_e2e_test;
using namespace svt_av1_e2e_test_vector;

/**
 * @brief SVT-AV1 encoder simple E2E test
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames.
 *
 * Expected result:
 * No error is reported in encoding progress. The output compressed data
 * is complete.
 *
 * Test coverage:
 * All test vectors
 */
class SvtAv1E2ESimpleTest : public SvtAv1E2ETestFramework {};

TEST_P(SvtAv1E2ESimpleTest, run_smoking_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ESimpleTest,
                        ::testing::ValuesIn(video_src_vectors));

/**
 * @brief SVT-AV1 encoder simple E2E test with save compressed data in file
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames. Save the compressed data into IVF file.
 *
 * Expected result:
 * No error is reported in encoding progress. The output compressed data
 * is saved into IVF file.
 *
 * Test coverage:
 * Smoking test vectors
 */
class SvtAv1E2ESimpleFileTest : public SvtAv1E2ETestFramework {
  protected:
    /** initialization for test */
    void init_test() override {
        output_file_ = new IvfFile("output.av1");
        SvtAv1E2ETestFramework::init_test();
    }
};

TEST_P(SvtAv1E2ESimpleFileTest, run_smoking_with_output_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ESimpleFileTest,
                        ::testing::ValuesIn(smoking_vectors));

/**
 * @brief SVT-AV1 encoder E2E test with save the reconstruction frames in
 * file
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames. Save the reconstruction frame data in file
 *
 * Expect result:
 * No error from encoding progress and reconstruction frame data saved in file.
 *
 * Test coverage:
 * Smoking test vectors
 */
class SvtAv1E2EReconFileTest : public SvtAv1E2ETestFramework {
  protected:
    /** initialization for test */
    void init_test() override {
        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_queue_ = create_frame_queue(param, "enc_recon.rcs");
        ASSERT_NE(recon_queue_, nullptr) << "can not create recon sink!!";
        if (recon_queue_)
            av1enc_ctx_.enc_params.recon_enabled = 1;
        SvtAv1E2ETestFramework::init_test();
    }
};

TEST_P(SvtAv1E2EReconFileTest, run_recon_collect_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EReconFileTest,
                        ::testing::ValuesIn(smoking_vectors));

/**
 * @brief SVT-AV1 encoder E2E test with save the reconstruction frames in
 * buffer list
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames. Save the reconstruction frame data in buffer list
 *
 * Expect result:
 * No error from encoding progress and reconstruction frame data saved in
 * buffer.
 *
 * Test coverage:
 * Smoking test vectors
 */
class SvtAv1E2EReconBufferTest : public SvtAv1E2ETestFramework {
  protected:
    /** initialization for test */
    void init_test() override {
        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_queue_ = create_frame_queue(param);
        ASSERT_NE(recon_queue_, nullptr) << "can not create recon sink!!";
        if (recon_queue_)
            av1enc_ctx_.enc_params.recon_enabled = 1;
        SvtAv1E2ETestFramework::init_test();
    }
};

TEST_P(SvtAv1E2EReconBufferTest, run_recon_collect_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EReconBufferTest,
                        ::testing::ValuesIn(smoking_vectors));

/**
 * @brief SVT-AV1 encoder E2E test with comparing the reconstructed frame with
 * output frame from decoder buffer list
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames. Collect the reconstructed frames and compared them with reference
 * decoder output.
 *
 * Expected result:
 * No error is reported in encoding progress. The reconstructed frame
 * data is same as the output frame from reference decoder.
 *
 * Test coverage:
 * All test vectors
 */
class SvtAv1E2EConformanceTest : public SvtAv1E2ETestFramework {
  protected:
    /** initialization for test */
    void init_test() override {
        // create recon sink before setup parameter of encoder
        VideoFrameParam param;
        memset(&param, 0, sizeof(param));
        param.format = video_src_->get_image_format();
        param.width = video_src_->get_width_with_padding();
        param.height = video_src_->get_height_with_padding();
        recon_queue_ = create_frame_queue(param);
        ASSERT_NE(recon_queue_, nullptr) << "can not create recon sink!!";
        if (recon_queue_)
            av1enc_ctx_.enc_params.recon_enabled = 1;

        // create reference decoder
        refer_dec_ = create_reference_decoder();
        ASSERT_NE(refer_dec_, nullptr) << "can not create reference decoder!!";

        collect_ = new PerformanceCollect(typeid(this).name());

        SvtAv1E2ETestFramework::init_test();
    }
};

TEST_P(SvtAv1E2EConformanceTest, run_conformance_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EConformanceTest,
                        ::testing::ValuesIn(comformance_test_vectors));

/**
 * @brief SVT-AV1 encoder E2E test with comparing the reconstructed frames with
 * output frames from decoder buffer list in longtime (3000 frames)
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with enc_mode 3, and encode the input YUV data
 * frames. Collect the reconstruction frames and compared with reference decoder
 * output
 *
 * Expect result:
 * No error from encoding progress and reconstruction frame data is same as the
 * output frame from reference decoder
 *
 * Test coverage:
 * Longtime test vectors
 */
class SvtAv1E2ELongTimeConformanceTest : public SvtAv1E2EConformanceTest {
  protected:
    /** initialization for test */
    void init_test() override {
        av1enc_ctx_.enc_params.enc_mode = 3;
        SvtAv1E2EConformanceTest::init_test();
    }
};

TEST_P(SvtAv1E2ELongTimeConformanceTest, run_conformance_test) {
    run_encode_process();
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ELongTimeConformanceTest,
                        ::testing::ValuesIn(longtime_comformance_test_vectors));

/* @brief SVT-AV1 encoder E2E test by comparing the reconstruction frames with
 * output frame from decoder buffer list, but found dead in linux
 *
 * Test strategy:
 * Setup SVT-AV1 encoder with default parameter, and encode the input YUV data
 * frames. Collect the reconstructed frames and compared them with reference
 * decoder output, and in progress the test will crashes with segment fault.
 *
 * Expected result:
 * The assert of death is true and pass the test
 *
 * Test coverage:
 * Special test vectors with resolution 952x512 and 952x704 in
 * src_resolution_death_test_vectors
 */
class SvtAv1E2EConformanceDeathTest : public SvtAv1E2EConformanceTest {};

TEST_P(SvtAv1E2EConformanceDeathTest, run_conformance_test) {
    testing::FLAGS_gtest_death_test_style = "threadsafe";
    ASSERT_DEATH(run_encode_process(), "");
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2EConformanceDeathTest,
                        ::testing::ValuesIn(src_resolution_death_test_vectors));

class SvtAv1E2ERepeatConformanceTest : public SvtAv1E2EConformanceTest {
  protected:
    SvtAv1E2ERepeatConformanceTest() {
        recon_storage_.clear();
    }
    virtual ~SvtAv1E2ERepeatConformanceTest() {
        for (FrameQueue* recon_queue : recon_storage_)
            delete recon_queue;
        recon_storage_.clear();
    }
    /** initialization for test */
    void init_test() override {
        SvtAv1E2EConformanceTest::init_test();
        // store the recon queue for compared with other encoder instances
        recon_storage_.push_back(recon_queue_);
    }
    void close_test() override {
        SvtAv1E2EConformanceTest::close_test();
        recon_queue_ = nullptr;
    }
    void repeat_encode_process(uint32_t times) {
        uint32_t left = times;
        // repeat run encode process
        while (left != 0) {
            if (left != times)  // first time is automatically initialized
                SetUp();
            run_encode_process();
            left--;
            if (left != 0)
                TearDown();
        }
        // check for frame
        FrameQueue* ref_queue = recon_storage_.back();
        for (FrameQueue* recon_queue : recon_storage_) {
            if (ref_queue != recon_queue)
                ASSERT_TRUE(ref_queue->compare(recon_queue))
                    << "frame queues compare failed";
        }
    }

  protected:
    std::vector<FrameQueue*> recon_storage_;
};

TEST_P(SvtAv1E2ERepeatConformanceTest, repeat_encode_process) {
    repeat_encode_process(5);
}

INSTANTIATE_TEST_CASE_P(SVT_AV1, SvtAv1E2ERepeatConformanceTest,
                        ::testing::ValuesIn(comformance_test_vectors));
