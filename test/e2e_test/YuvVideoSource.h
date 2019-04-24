/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file YuvVideoSource.h
 *
 * @brief YuvVideoSource class for reading yuv file.
 *
 * @author Cidana-Ryan
 *
 ******************************************************************************/
#ifndef _SVT_TEST_YUV_VIDEO_SOURCE_H_
#define _SVT_TEST_YUV_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"
namespace svt_av1_video_source {
class YuvVideoSource : public VideoSource {
  public:
    YuvVideoSource(const std::string &file_name, const VideoColorFormat format,
                   const uint32_t width, const uint32_t height,
                   const uint8_t bit_depth);
    ~YuvVideoSource();
    /*!\brief Prepare stream. */
    EbErrorType open_source() override;
    /*!\brief Close stream. */
    void close_source() override;
    /*!\brief Get next frame. */
    EbSvtIOFormat *get_next_frame() override;
    /*!\brief Get frame ny index. */
    EbSvtIOFormat *get_frame_by_index(const uint32_t index) override {
        return nullptr;
    };

  private:
    uint32_t read_input_frame();
    std::string file_name_;
    FILE *file_handle_;
    uint32_t frame_length_;
    uint32_t file_length_;
};
}  // namespace svt_av1_video_source
#endif  //_SVT_TEST_VIDEO_SOURCE_H_
