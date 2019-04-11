/*
 * Copyright(c) 2019 Intel Corporation
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

class Y4MVideoSource : public VideoSource {
  public:
    Y4MVideoSource(const std::string &file_name, const VideoImageFormat format,
                   const uint32_t width, const uint32_t height,
                   const uint8_t bit_depth);
    ~Y4MVideoSource();
    /*!\brief Prepare stream. */
    EbErrorType open_source() override;
    /*!\brief Close stream. */
    virtual void close_source() override;
    /*!\brief Get next frame. */
    EbSvtIOFormat *get_next_frame() override;
    /*!\brief Get frame ny index. */
    EbSvtIOFormat *get_frame_by_index(const uint32_t index) override;

  private:
    EbErrorType parse_file_info();
    uint32_t read_input_frame();
    std::string file_name_;
    FILE *file_handle_;
    uint32_t file_length_;
    uint32_t frame_length_;
    uint32_t header_length_;
#ifdef ENABLE_DEBUG_MONITOR
    VideoMonitor *monitor;
#endif
};
#endif  //_SVT_TEST_Y4M_VIDEO_SOURCE_H_
