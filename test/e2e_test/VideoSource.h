/*
 * Copyright(c) 2019 Intel Corporation
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
#include "EbSvtAv1Enc.h"
#include "VideoFrame.h"

namespace svt_av1_video_source {
static std::string get_vector_path() {
    const char *const path = getenv("SVT_AV1_TEST_VECTOR_PATH");
    if (path == nullptr) {
        return ".";
    }
    return path;
}

class VideoSource {
  public:
    virtual ~VideoSource() {
        deinit_frame_buffer();
    };
    /*!\brief Prepare stream. */
    virtual EbErrorType open_source() = 0;
    /*!\brief Close stream. */
    virtual void close_source() = 0;
    /*!\brief Get next frame. */
    virtual EbSvtIOFormat *get_next_frame() = 0;
    /*!\brief Get frame by index. */
    virtual EbSvtIOFormat *get_frame_by_index(const uint32_t index) = 0;
    /*!\brief Get current frame index. */
    virtual uint32_t get_frame_index() {
        return current_frame_index_;
    };
    /*!\brief Get current frame size in bytes. */
    virtual uint32_t get_frame_size() {
        return frame_size_;
    };
    /*!\brief Get video width with padding. SVT-AV1 support only multiple of 8
     * resolutions. We will add some padding on right side if needed.  */
    virtual uint32_t get_width_with_padding() {
        return width_with_padding_;
    };
    /*!\brief Get video height with oadding.SVT-AV1 support only multiple of 8
     * resolutions. We will add some padding on bottom if needed.*/
    virtual uint32_t get_height_with_padding() {
        return height_with_padding_;
    };
    /*!\brief Get video bit_depth */
    virtual uint32_t get_bit_depth() {
        return bit_depth_;
    };
    /*!\brief Get video image format. */
    virtual VideoImageFormat get_image_format() {
        return image_format_;
    }
    /*!\brief Get total frame count. */
    virtual uint32_t get_frame_count() {
        return frame_count_;
    }
    /*!\brief If the return value is true, video source will use svt compressed
     * 10bit mode for output . */
    virtual bool get_compressed_10bit_mode() {
        return svt_compressed_2bit_plane;
    }

  protected:
    bool is_ten_bit_mode() {
        if (image_format_ == IMG_FMT_420P10_PACKED ||
            image_format_ == IMG_FMT_422P10_PACKED ||
            image_format_ == IMG_FMT_444P10_PACKED) {
            return true;
        }
        return false;
    }

    void deinit_frame_buffer() {
        if (frame_buffer_ == nullptr)
            return;

        if (frame_buffer_->luma != nullptr) {
            free(frame_buffer_->luma);
            frame_buffer_->luma = nullptr;
        }
        if (frame_buffer_->cb != nullptr) {
            free(frame_buffer_->cb);
            frame_buffer_->cb = nullptr;
        }
        if (frame_buffer_->cr != nullptr) {
            free(frame_buffer_->cr);
            frame_buffer_->cr = nullptr;
        }
        if (frame_buffer_->luma_ext != nullptr) {
            free(frame_buffer_->luma_ext);
            frame_buffer_->luma_ext = nullptr;
        }
        if (frame_buffer_->cb_ext != nullptr) {
            free(frame_buffer_->cb_ext);
            frame_buffer_->cb_ext = nullptr;
        }
        if (frame_buffer_->cr_ext != nullptr) {
            free(frame_buffer_->cr_ext);
            frame_buffer_->cr_ext = nullptr;
        }
        free(frame_buffer_);
        frame_buffer_ = nullptr;
    }
    virtual EbErrorType init_frame_buffer() {
        // Determine size of each plane
        uint32_t luma_size = width_with_padding_ * height_with_padding_;
        uint32_t chroma_size = 0;

        switch (image_format_) {
        case IMG_FMT_420P10_PACKED:
        case IMG_FMT_420: {
            chroma_size = luma_size >> 2;
        } break;
        case IMG_FMT_422P10_PACKED:
        case IMG_FMT_422: {
            chroma_size = luma_size >> 1;
        } break;
        case IMG_FMT_444P10_PACKED:
        case IMG_FMT_444: {
            chroma_size = luma_size;
        } break;
        default: {
            chroma_size = luma_size >> 2;
        } break;
        }

        // Determine
        if (frame_buffer_ == nullptr)
            frame_buffer_ = (EbSvtIOFormat *)malloc(sizeof(EbSvtIOFormat));
        else {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->width = width_with_padding_;
        frame_buffer_->height = height_with_padding_;
        frame_buffer_->origin_x = 0;
        frame_buffer_->origin_y = 0;

        // SVT-AV1 use pixel size as stride?
        frame_buffer_->y_stride = luma_size;
        frame_buffer_->cb_stride = chroma_size;
        frame_buffer_->cr_stride = chroma_size;

        if (is_ten_bit_mode() && !svt_compressed_2bit_plane) {
            luma_size *= 2;
            chroma_size *= 2;
        }

        frame_buffer_->luma = (uint8_t *)malloc(luma_size);
        if (!frame_buffer_->luma) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cb = (uint8_t *)malloc(chroma_size);
        if (!frame_buffer_->cb) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cr = (uint8_t *)malloc(chroma_size);
        if (!frame_buffer_->cr) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        if (is_ten_bit_mode() && svt_compressed_2bit_plane) {
            frame_buffer_->luma_ext = (uint8_t *)malloc(luma_size / 4);
            if (!frame_buffer_->luma_ext) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cb_ext = (uint8_t *)malloc(chroma_size / 4);
            if (!frame_buffer_->cb_ext) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cr_ext = (uint8_t *)malloc(chroma_size / 4);
            if (!frame_buffer_->cr_ext) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }
        } else {
            frame_buffer_->luma_ext = nullptr;
            frame_buffer_->cb_ext = nullptr;
            frame_buffer_->cr_ext = nullptr;
        }

        return EB_ErrorNone;
    }
    uint32_t width_;
    uint32_t height_;
    uint32_t width_with_padding_;
    uint32_t height_with_padding_;
    uint32_t bit_depth_;
    uint32_t frame_count_;
    int32_t current_frame_index_;
    uint32_t frame_size_;
    EbSvtIOFormat *frame_buffer_;
    VideoImageFormat image_format_;
    bool svt_compressed_2bit_plane;
};

}  // namespace svt_av1_video_source

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
