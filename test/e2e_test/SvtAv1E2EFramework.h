/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file SvtAv1E2EFramework.h
 *
 * @brief Defines a test framework for End to End test
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#ifndef _SVT_AV1_E2E_FRAMEWORK_H_
#define _SVT_AV1_E2E_FRAMEWORK_H_

#include "E2eTestVectors.h"
#include "ReconSink.h"
#include "PerformanceCollect.h"
#include "CompareTools.h"

using namespace svt_av1_e2e_tools;
using namespace svt_av1_video_source;
class RefDecoder;
extern RefDecoder *create_reference_decoder();

/** @defgroup svt_av1_e2e_test Test framework for E2E test
 *  Defines the framework body of E2E test for the mainly test progress
 *  @{
 */
namespace svt_av1_e2e_test {

#define INPUT_SIZE_576p_TH 0x90000    // 0.58 Million
#define INPUT_SIZE_1080i_TH 0xB71B0   // 0.75 Million
#define INPUT_SIZE_1080p_TH 0x1AB3F0  // 1.75 Million
#define INPUT_SIZE_4K_TH 0x29F630     // 2.75 Million
#define EB_OUTPUTSTREAMBUFFERSIZE_MACRO(resolution_size) \
    ((resolution_size) < (INPUT_SIZE_1080i_TH)           \
         ? 0x1E8480                                      \
         : (resolution_size) < (INPUT_SIZE_1080p_TH)     \
               ? 0x2DC6C0                                \
               : (resolution_size) < (INPUT_SIZE_4K_TH) ? 0x2DC6C0 : 0x2DC6C0)

#define LONG_ENCODE_FRAME_ENCODE 4000
#define SPEED_MEASUREMENT_INTERVAL 2000
#define START_STEADY_STATE 1000
#define AV1_FOURCC 0x31305641  // used for ivf header
#define IVF_STREAM_HEADER_SIZE 32
#define IVF_FRAME_HEADER_SIZE 12
#define OBU_FRAME_HEADER_SIZE 3
#define TD_SIZE 2
static __inline void mem_put_le32(void *vmem, int32_t val) {
    uint8_t *mem = (uint8_t *)vmem;
    mem[0] = (uint8_t)((val >> 0) & 0xff);
    mem[1] = (uint8_t)((val >> 8) & 0xff);
    mem[2] = (uint8_t)((val >> 16) & 0xff);
    mem[3] = (uint8_t)((val >> 24) & 0xff);
}
#define MEM_VALUE_T_SZ_BITS (sizeof(MEM_VALUE_T) << 3)
static __inline void mem_put_le16(void *vmem, int32_t val) {
    uint8_t *mem = (uint8_t *)vmem;
    mem[0] = (uint8_t)((val >> 0) & 0xff);
    mem[1] = (uint8_t)((val >> 8) & 0xff);
}

using namespace svt_av1_e2e_test_vector;

/** SvtAv1Context is a set of test contexts in whole test progress */
typedef struct {
    EbComponentType
        *enc_handle; /**< encoder handle, created from encoder library */
    EbSvtAv1EncConfiguration enc_params; /**< encoder parameter set */
    EbBufferHeaderType
        *output_stream_buffer; /**< output buffer of encoder in test */
    EbBufferHeaderType
        *input_picture_buffer; /**< input buffer of encoder in test */
} SvtAv1Context;

/** SvtAv1E2ETestBase is a basic class for only impelmention of setup, teardown,
 * init and close with normal setting */
class SvtAv1E2ETestBase : public ::testing::TestWithParam<TestVideoVector> {
  public:
    SvtAv1E2ETestBase();
    virtual ~SvtAv1E2ETestBase();

  protected:
    virtual void SetUp() override;
    virtual void TearDown() override;
    /** initialization for test */
    virtual void init_test();
    /** close for test */
    virtual void close_test();
    /** test processing body */
    virtual void run_encode_process() = 0;

  public:
    static VideoSource *prepare_video_src(const TestVideoVector &vector);

  protected:
    void trans_src_param();

  protected:
    VideoSource *video_src_; /**< video source context */
    SvtAv1Context ctxt_;     /**< AV1 encoder context */
};

/** SvtAv1E2ETestFramework is a class with impelmention of video source control,
 * encoding progress, decoding progress, data collection and data comparision */
class SvtAv1E2ETestFramework : public SvtAv1E2ETestBase {
  public:
    typedef struct IvfFile {
        FILE *file;
        uint64_t byte_count_since_ivf;
        uint64_t ivf_count;
        IvfFile(std::string path);
        ~IvfFile() {
            if (file) {
                fclose(file);
                file = nullptr;
            }
            byte_count_since_ivf = 0;
            ivf_count = 0;
        }
    } IvfFile;

  protected:
    SvtAv1E2ETestFramework();
    virtual ~SvtAv1E2ETestFramework();

  protected:
    /** initialization for test */
    virtual void init_test() override;
    /** test processing body */
    void run_encode_process() override;

  private:
    /** write ivf header to output file */
    void write_output_header();
    /** write compressed data into file
     * @param output  compressed data from encoder
     */
    void write_compress_data(const EbBufferHeaderType *output);
    /** process compressed data by write to file for send to decoder
     * @param data  compressed data from encoder
     */
    void process_compress_data(const EbBufferHeaderType *data);
    /** send compressed data to decoder
     * @param data  compressed data from encoder, single OBU
     * @param size  size of compressed data
     */
    void decode_compress_data(const uint8_t *data, const uint32_t size);
    /** check video frame psnr with source
     * @param frame  video frame from reference decoder
     */
    void check_psnr(const VideoFrame &frame);

  protected:
    /** get reconstruction frame from encoder, it should call after send data
     * @param is_eos  flag of recon frames is eos
     * into decoder */
    virtual void get_recon_frame(bool &is_eos);

  protected:
    ReconSink *recon_sink_; /**< reconstruction frame collection */
    RefDecoder *refer_dec_; /**< reference decoder context */
    IvfFile *output_file_;  /**< file handle for save encoder output data */
    uint8_t obu_frame_header_size_; /**< size of obu frame header */
    PerformanceCollect *collect_;   /**< performance and time collection*/
    VideoSource *psnr_src_;         /**< video source context for psnr */
    ICompareSink *ref_compare_; /**< sink of reference to compare with recon*/
    svt_av1_e2e_tools::PsnrStatistics
        pnsr_statistics_; /**< psnr statistics recorder.*/
    uint64_t total_enc_out_;
};

}  // namespace svt_av1_e2e_test
/** @} */  // end of svt_av1_e2e_test_vector

#endif  //_SVT_AV1_E2E_FRAMEWORK_H_
