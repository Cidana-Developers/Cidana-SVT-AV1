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

/**
 * @brief      Video source abstract class.
 */
class VideoSource {
  public:
    VideoSource(const VideoColorFormat format, const uint32_t width,
                const uint32_t height, const uint8_t bit_depth,
                const bool use_compressed_2bit_plan_output)
        : width_(width),
          width_with_padding_(width),
          height_(height),
          height_with_padding_(height),
          bit_depth_(bit_depth),
          frame_count_(0),
          current_frame_index_(-1),
          frame_size_(0),
          frame_buffer_(nullptr),
          image_format_(format),
          svt_compressed_2bit_plane_(false) {
        if (bit_depth_ > 8 && use_compressed_2bit_plan_output)
            svt_compressed_2bit_plane_ = true;
        else
            svt_compressed_2bit_plane_ = false;
    };
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
    virtual VideoColorFormat get_image_format() {
        return image_format_;
    }
    /*!\brief Get total frame count. */
    virtual uint32_t get_frame_count() {
        return frame_count_;
    }
    /*!\brief If the return value is true, video source will use svt compressed
     * 10bit mode for output . */
    virtual bool get_compressed_10bit_mode() {
        return svt_compressed_2bit_plane_;
    }

  protected:
    bool is_10bit_mode() {
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

        if (frame_buffer_ == nullptr) {
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

        if (is_10bit_mode() && !svt_compressed_2bit_plane_) {
            luma_size *= 2;
            chroma_size *= 2;
        }

        frame_buffer_->luma = (uint8_t *)malloc(luma_size);
        if (frame_buffer_->luma == nullptr) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cb = (uint8_t *)malloc(chroma_size);
        if (frame_buffer_->cb == nullptr) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cr = (uint8_t *)malloc(chroma_size);
        if (frame_buffer_->cr == nullptr) {
            deinit_frame_buffer();
            return EB_ErrorInsufficientResources;
        }

        if (is_10bit_mode() && svt_compressed_2bit_plane_) {
            frame_buffer_->luma_ext = (uint8_t *)malloc(luma_size / 4);
            if (frame_buffer_->luma_ext == nullptr) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cb_ext = (uint8_t *)malloc(chroma_size / 4);
            if (frame_buffer_->cb_ext == nullptr) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cr_ext = (uint8_t *)malloc(chroma_size / 4);
            if (frame_buffer_->cr_ext == nullptr) {
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
    VideoColorFormat image_format_;
    bool svt_compressed_2bit_plane_;
};  // namespace svt_av1_video_source

class VideoFileSource : public VideoSource {
  public:
    VideoFileSource(const std::string &file_name, const VideoColorFormat format,
                    const uint32_t width, const uint32_t height,
                    const uint8_t bit_depth,
                    const bool use_compressed_2bit_plan_output)
        : VideoSource(format, width, height, bit_depth,
                      use_compressed_2bit_plan_output),
          file_name_(file_name),
          file_handle_(nullptr) {
        if (width_ % 8 != 0)
            width_with_padding_ += +(8 - width_ % 8);
        if (height_ % 8 != 0)
            height_with_padding_ += (8 - height_ % 8);
    }
    virtual ~VideoFileSource() {
    }
    /**
     * @brief      Use this funcion to get vector path defined by envrionment
     * variable SVT_AV1_TEST_VECTOR_PATH, or it will return a default path.
     *
     * @return     The vectors path.
     */
    static std::string get_vector_path() {
        const char *const path = getenv("SVT_AV1_TEST_VECTOR_PATH");
        if (path == nullptr) {
#ifdef _WIN32
            return "../../../../test/vectors";
#else
            return "../../test/vectors";
#endif  // _WIN32
        }
        return path;
    }

  protected:
    virtual uint32_t read_input_frame() {
        uint32_t filled_len = 0;
        if (file_handle_ == nullptr) {
            printf("Error file handle\r\n");
            return 0;
        }

        if (feof(file_handle_) != 0) {
            printf("Reach file end\r\n");
            return 0;
        }
        int chroma_down_size = 2;
        int width_downsize = 1;
        int height_downsize = 1;
        int pixel_byte_size = 1;

        switch (image_format_) {
        case IMG_FMT_420: {
            chroma_down_size = 2;
            width_downsize = 1;
            height_downsize = 1;
            pixel_byte_size = 1;
            break;
        }
        case IMG_FMT_422: {
            chroma_down_size = 1;
            width_downsize = 1;
            height_downsize = 0;
            pixel_byte_size = 1;
            break;
        }
        case IMG_FMT_444: {
            chroma_down_size = 0;
            width_downsize = 0;
            height_downsize = 0;
            pixel_byte_size = 1;
            break;
        }
        case IMG_FMT_420P10_PACKED: {
            chroma_down_size = 2;
            width_downsize = 1;
            height_downsize = 1;
            pixel_byte_size = 2;
            break;
        }
        case IMG_FMT_422P10_PACKED: {
            chroma_down_size = 1;
            width_downsize = 1;
            height_downsize = 0;
            pixel_byte_size = 2;
            break;
        }
        case IMG_FMT_444P10_PACKED: {
            chroma_down_size = 0;
            width_downsize = 0;
            height_downsize = 0;
            pixel_byte_size = 2;
            break;
        }
        default: break;
        }

        // SVT-AV1 use pixel size as stride?
        frame_buffer_->y_stride = width_with_padding_;
        frame_buffer_->cb_stride = (width_with_padding_ >> width_downsize);
        frame_buffer_->cr_stride = (width_with_padding_ >> width_downsize);

        // Read raw data from file
        const uint32_t bottom_padding = height_with_padding_ - height_;
        const uint32_t righ_padding = width_with_padding_ - width_;
        size_t read_len = 0;
        uint32_t i;
        if (bit_depth_ <= 8 ||
            (bit_depth_ > 8 && !svt_compressed_2bit_plane_)) {
            uint8_t *eb_input_ptr = nullptr;
            // Y
            eb_input_ptr = frame_buffer_->luma;
            for (i = 0; i < height_; ++i) {
                read_len = fread(
                    eb_input_ptr, 1, width_ * pixel_byte_size, file_handle_);
                if (read_len != width_ * pixel_byte_size)
                    return 0;  // read file error.

                memset(eb_input_ptr + width_ * pixel_byte_size,
                       0,
                       righ_padding * pixel_byte_size);
                eb_input_ptr += frame_buffer_->y_stride * pixel_byte_size;
                filled_len += frame_buffer_->y_stride * pixel_byte_size;
            }
            for (i = 0; i < bottom_padding; ++i) {
                memset(eb_input_ptr, 0, width_with_padding_ * pixel_byte_size);
                eb_input_ptr += frame_buffer_->y_stride * pixel_byte_size;
                filled_len += frame_buffer_->y_stride * pixel_byte_size;
            }
            // Cb
            eb_input_ptr = frame_buffer_->cb;
            for (i = 0; i < (height_ >> height_downsize); ++i) {
                read_len = fread(eb_input_ptr,
                                 1,
                                 (width_ >> width_downsize) * pixel_byte_size,
                                 file_handle_);
                if (read_len != (width_ >> width_downsize) * pixel_byte_size)
                    return 0;  // read file error.

                memset(
                    eb_input_ptr + (width_ >> width_downsize) * pixel_byte_size,
                    0,
                    (righ_padding >> width_downsize) * pixel_byte_size);
                eb_input_ptr += frame_buffer_->cb_stride * pixel_byte_size;
                filled_len += frame_buffer_->cb_stride * pixel_byte_size;
            }
            for (i = 0; i < (bottom_padding >> height_downsize); ++i) {
                memset(
                    eb_input_ptr,
                    0,
                    (width_with_padding_ >> width_downsize) * pixel_byte_size);
                eb_input_ptr += frame_buffer_->cb_stride * pixel_byte_size;
                filled_len += frame_buffer_->cb_stride * pixel_byte_size;
            }

            // Cr
            eb_input_ptr = frame_buffer_->cr;

            for (i = 0; i < (height_ >> height_downsize); ++i) {
                read_len = fread(eb_input_ptr,
                                 1,
                                 (width_ >> width_downsize) * pixel_byte_size,
                                 file_handle_);
                if (read_len != (width_ >> width_downsize) * pixel_byte_size)
                    return 0;  // read file error.

                memset(
                    eb_input_ptr + (width_ >> width_downsize) * pixel_byte_size,
                    0,
                    (righ_padding >> width_downsize) * pixel_byte_size);
                eb_input_ptr += frame_buffer_->cr_stride * pixel_byte_size;
                filled_len += frame_buffer_->cr_stride * pixel_byte_size;
            }
            for (i = 0; i < (bottom_padding >> height_downsize); ++i) {
                memset(
                    eb_input_ptr,
                    0,
                    (width_with_padding_ >> width_downsize) * pixel_byte_size);
                eb_input_ptr += frame_buffer_->cr_stride * pixel_byte_size;
                filled_len += frame_buffer_->cr_stride * pixel_byte_size;
            }
        } else if (bit_depth_ > 8 && svt_compressed_2bit_plane_) {
            uint8_t *eb_input_ptr = nullptr;
            uint8_t *eb_ext_input_ptr = nullptr;
            // Y
            uint32_t j = 0;
            uint16_t pix = 0;
            eb_input_ptr = frame_buffer_->luma;
            eb_ext_input_ptr = frame_buffer_->luma_ext;
            for (i = 0; i < height_; ++i) {
                int j = 0;
                for (j = 0; j < width_; ++j) {
                    // Get one pixel
                    if (2 != fread(&pix, 1, 2, file_handle_)) {
                        return 0;
                    }
                    eb_input_ptr[j] = (uint8_t)(pix >> 2);
                    eb_ext_input_ptr[j / 4] &= (pix & 0x3) << (j % 4);
                }

                for (; j < width_ + righ_padding; ++j) {
                    eb_input_ptr[j] = 0;
                    eb_ext_input_ptr[j / 4] = 0;
                }
                eb_input_ptr += frame_buffer_->y_stride;
                eb_ext_input_ptr += frame_buffer_->y_stride / 4;
                filled_len += frame_buffer_->y_stride * 5 / 4;
            }
            for (i = 0; i < bottom_padding; ++i) {
                memset(eb_input_ptr, 0, width_with_padding_);
                memset(eb_ext_input_ptr, 0, width_with_padding_ / 4);
                eb_input_ptr += frame_buffer_->y_stride;
                eb_ext_input_ptr += frame_buffer_->y_stride / 4;
                filled_len += frame_buffer_->y_stride * 5 / 4;
            }
            // Cb
            eb_input_ptr = frame_buffer_->cb;
            eb_ext_input_ptr = frame_buffer_->cb_ext;
            for (i = 0; i < (height_ >> height_downsize); ++i) {
                int j = 0;
                for (j = 0; j < (width_ >> width_downsize); ++j) {
                    // Get one pixel
                    if (2 != fread(&pix, 1, 2, file_handle_)) {
                        return 0;
                    }
                    eb_input_ptr[j] = (uint8_t)(pix >> 2);
                    eb_ext_input_ptr[j / 4] &= (pix & 0x3) << (j % 4);
                }

                for (; j < ((width_ + righ_padding) >> width_downsize); ++j) {
                    eb_input_ptr[j] = 0;
                    eb_ext_input_ptr[j / 4] = 0;
                }
                eb_input_ptr += frame_buffer_->cb_stride;
                eb_ext_input_ptr += frame_buffer_->cb_stride / 4;
                filled_len += frame_buffer_->cb_stride * 5 / 4;
            }
            for (i = 0; i<bottom_padding>> height_downsize; ++i) {
                memset(
                    eb_input_ptr, 0, (width_with_padding_ >> width_downsize));
                memset(eb_ext_input_ptr,
                       0,
                       (width_with_padding_ >> width_downsize) / 4);
                eb_input_ptr += frame_buffer_->cb_stride;
                eb_ext_input_ptr += frame_buffer_->cb_stride / 4;
                filled_len += frame_buffer_->cb_stride * 5 / 4;
            }

            // Cr
            eb_input_ptr = frame_buffer_->cr;
            eb_ext_input_ptr = frame_buffer_->cr_ext;
            for (i = 0; i < (height_ >> height_downsize); ++i) {
                int j = 0;
                for (j = 0; j < (width_ >> width_downsize); ++j) {
                    // Get one pixel
                    if (2 != fread(&pix, 1, 2, file_handle_)) {
                        return 0;
                    }
                    eb_input_ptr[j] = (uint8_t)(pix >> 2);
                    eb_ext_input_ptr[j / 4] &= (pix & 0x3) << (j % 4);
                }

                for (; j < ((width_ + righ_padding) >> width_downsize); ++j) {
                    eb_input_ptr[j] = 0;
                    eb_ext_input_ptr[j / 4] = 0;
                }
                eb_input_ptr += frame_buffer_->cr_stride;
                eb_ext_input_ptr += frame_buffer_->cr_stride / 4;
                filled_len += frame_buffer_->cr_stride * 5 / 4;
            }
            for (i = 0; i<bottom_padding>> height_downsize; ++i) {
                memset(
                    eb_input_ptr, 0, (width_with_padding_ >> width_downsize));
                memset(eb_ext_input_ptr,
                       0,
                       (width_with_padding_ >> width_downsize) / 4);
                eb_input_ptr += frame_buffer_->cr_stride;
                eb_ext_input_ptr += frame_buffer_->cr_stride / 4;
                filled_len += frame_buffer_->cr_stride * 5 / 4;
            }
        }

        //     printf("Target:[%dx%d:%d],Read[%d]\r\n",
        //            width_with_padding_,
        //            height_with_padding_,
        //            (width_with_padding_ * height_with_padding_) +
        //                2 * ((width_with_padding_ * height_with_padding_) >>
        //                     chroma_down_size),
        //            filled_len);

        return filled_len;
    }

    std::string file_name_;
    FILE *file_handle_;
    uint32_t file_length_;
};
}  // namespace svt_av1_video_source

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
