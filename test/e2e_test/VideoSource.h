/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file VideoSource.h
 *
 * @brief Abstract class for reading video source file.
 *
 * @author Cidana-Ryan
 *
 ******************************************************************************/

#ifndef _SVT_TEST_VIDEO_SOURCE_H_
#define _SVT_TEST_VIDEO_SOURCE_H_
#include <string>
#include <vector>
#include "EbSvtAv1Enc.h"
#include "VideoFrame.h"
#ifdef ENABLE_DEBUG_MONITOR
#include "VideoMonitor.h"
#endif

namespace svt_av1_video_source {

/**
 * @brief      Video source abstract class.
 */
class VideoSource {
  public:
    VideoSource(const VideoColorFormat format, const uint32_t width,
                const uint32_t height, const uint8_t bit_depth,
                const bool use_compressed_2bit_plane_output);
    virtual ~VideoSource();
    /*!\brief Prepare stream. */
    virtual EbErrorType open_source(const uint32_t init_pos,
                                    const uint32_t frame_count) = 0;
    /*!\brief Close stream. */
    virtual void close_source() = 0;
    /*!\brief Get next frame. */
    virtual EbSvtIOFormat *get_next_frame() = 0;
    /*!\brief Get frame by index. */
    virtual EbSvtIOFormat *get_frame_by_index(const uint32_t index) = 0;
    /*!\brief Get current frame index. */
    virtual uint32_t get_frame_index() const {
        return current_frame_index_;
    };
    /*!\brief Get current frame size in bytes. */
    virtual uint32_t get_frame_size() const {
        return frame_size_;
    };
    /*!\brief Get video width with padding. SVT-AV1 support only multiple of 8
     * resolutions. We will add some padding on right side if needed.  */
    virtual uint32_t get_width_with_padding() const {
        return width_with_padding_;
    };
    /*!\brief Get video height with oadding.SVT-AV1 support only multiple of 8
     * resolutions. We will add some padding on bottom if needed.*/
    virtual uint32_t get_height_with_padding() const {
        return height_with_padding_;
    };
    /*!\brief Get video bit_depth */
    virtual uint32_t get_bit_depth() const {
        return bit_depth_;
    };
    /*!\brief Get video image format. */
    virtual VideoColorFormat get_image_format() const {
        return image_format_;
    }
    /*!\brief Get total frame count. */
    virtual uint32_t get_frame_count() const {
        return frame_count_;
    }
    /*!\brief If the return value is true, video source will use svt compressed
     * 10bit mode for output . */
    virtual bool get_compressed_10bit_mode() const {
        return svt_compressed_2bit_plane_;
    }
    /*!\brief Get the frame qp by index */
    virtual uint32_t get_frame_qp(const uint32_t index) const {
        if (index >= frame_qp_list_.size())
            return INVALID_QP;
        return frame_qp_list_.at(index);
    }

  protected:
    bool is_10bit_mode();
    void deinit_frame_buffer();
    virtual EbErrorType init_frame_buffer();
    uint32_t width_;
    uint32_t height_;
    uint32_t width_with_padding_;
    uint32_t height_with_padding_;
    uint32_t bit_depth_;
    uint32_t init_pos_;
    uint32_t frame_count_;
    int32_t current_frame_index_;
    uint32_t frame_size_;
    EbSvtIOFormat *frame_buffer_;
    VideoColorFormat image_format_;
    bool svt_compressed_2bit_plane_;
    std::vector<uint32_t> frame_qp_list_;
};

/**
 * @brief      Video file source abstract class, use to read image from file.
 */
class VideoFileSource : public VideoSource {
  public:
    VideoFileSource(const std::string &file_name, const VideoColorFormat format,
                    const uint32_t width, const uint32_t height,
                    const uint8_t bit_depth,
                    const bool use_compressed_2bit_plane_output);
    virtual ~VideoFileSource();
    /**
     * @brief      Use this funcion to get vector path defined by envrionment
     * variable SVT_AV1_TEST_VECTOR_PATH, or it will return a default path.
     *
     * @return     The vectors path.
     */
    static std::string get_vector_path();

    /*!\brief Prepare stream. */
    EbErrorType open_source(const uint32_t init_pos,
                            const uint32_t frame_count) override;
    /*!\brief Close stream. */
    void close_source() override;
    /*!\brief Get next frame. */
    EbSvtIOFormat *get_next_frame() override;
    /*!\brief Get frame by index. */
    EbSvtIOFormat *get_frame_by_index(const uint32_t index) override;

  protected:
    virtual uint32_t read_input_frame();

    /*!\brief Sub class need to implement this function, file_frames_ and
     * frame_length_ must be init in this function. */
    virtual EbErrorType parse_file_info() = 0;
    virtual EbErrorType seek_to_frame(const uint32_t index) = 0;
    std::string file_name_;
    FILE *file_handle_;
    uint32_t file_length_;
    uint32_t frame_length_;
    uint32_t file_frames_;
#ifdef ENABLE_DEBUG_MONITOR
    VideoMonitor *monitor;
#endif
};
}  // namespace svt_av1_video_source

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
