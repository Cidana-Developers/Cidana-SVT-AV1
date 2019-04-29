/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file Y4mVideoSource.h
 *
 * @brief Y4mVideoSource class for reading y4m file.
 *
 * @author Cidana-Ryan
 *
 ******************************************************************************/
#ifndef _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#define _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"
#ifdef ENABLE_DEBUG_MONITOR
#include "VideoMonitor.h"
#endif
namespace svt_av1_video_source {
class Y4MVideoSource : public VideoFileSource {
  public:
    Y4MVideoSource(const std::string &file_name, const VideoColorFormat format,
                   const uint32_t width, const uint32_t height,
                   const uint8_t bit_depth,
                   const bool use_compressed_2bit_plan_output);
    ~Y4MVideoSource();
    /*!\brief Prepare stream. */
    EbErrorType open_source(const uint32_t init_pos,
                            const uint32_t frame_count) override;
    /*!\brief Close stream. */
    void close_source() override;
    /*!\brief Get next frame. */
    EbSvtIOFormat *get_next_frame() override;
    /*!\brief Get frame by index. */
    EbSvtIOFormat *get_frame_by_index(const uint32_t index) override;

  private:
    EbErrorType parse_file_info();
    uint32_t read_input_frame() override;
    uint32_t frame_length_;
    uint32_t header_length_;
#ifdef ENABLE_DEBUG_MONITOR
    VideoMonitor *monitor;
#endif
};
}  // namespace svt_av1_video_source
#endif  //_SVT_TEST_Y4M_VIDEO_SOURCE_H_
