#ifndef _SVT_TEST_VIDEO_SOURCE_H_
#define _SVT_TEST_VIDEO_SOURCE_H_
#include "EbSvtAv1Enc.h"

typedef enum VideoImageFormat {
    IMG_FMT_420JPEG,
    IMG_FMT_420PALDV,
    IMG_FMT_420,
    IMG_FMT_422,
    IMG_FMT_444
} VideoImageFormat;

// Abstract base class for test video source
class VideoSource {
  public:
    virtual ~VideoSource(){};
    // Prepare stream.
    virtual EbErrorType open_source() = 0;
    // Get next frame.
    virtual EbSvtIOFormat *get_next_frame() = 0;
    // Get current frame index.
    virtual uint32_t get_frame_index() {
        return frame_count_;
    };
    // Get current frame size in byte
    virtual uint32_t get_frame_size() {
        return frame_size_;
    };
    // Get video width
    virtual uint32_t get_width() {
        return width_;
    };
    // Get video height
    virtual uint32_t get_height() {
        return height_;
    };
    // Get video bit_depth
    virtual uint32_t get_bit_depth() {
        return bit_depth_;
    };
    // Get video image format.
    virtual VideoImageFormat get_image_format() {
        return image_format_;
    }

  protected:
    virtual EbErrorType allocate_fream_buffer() {
        // Determine size of each plane
        const size_t luma_8bit_size = width_ * height_;
        const size_t chroma8bitSize = luma_8bit_size >> 2;

        // Determine
        if (frame_buffer_ == nullptr)
            frame_buffer_ = (EbSvtIOFormat *)malloc(sizeof(EbSvtIOFormat));
        else
            return EB_ErrorNone;

        frame_buffer_->luma = (uint8_t *)malloc(luma_8bit_size);
        if (!frame_buffer_->luma) {
            free(frame_buffer_);
            frame_buffer_ = nullptr;
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cb = (uint8_t *)malloc(chroma8bitSize);
        if (!frame_buffer_->cb) {
            free(frame_buffer_->luma);
            free(frame_buffer_);
            frame_buffer_ = nullptr;
            return EB_ErrorInsufficientResources;
        }

        frame_buffer_->cr = (uint8_t *)malloc(chroma8bitSize);
        if (!frame_buffer_->cr) {
            free(frame_buffer_->cb);
            free(frame_buffer_->luma);
            free(frame_buffer_);
            frame_buffer_ = nullptr;
            return EB_ErrorInsufficientResources;
        }

        return EB_ErrorNone;
    }
    uint32_t width_;
    uint32_t height_;
    uint32_t bit_depth_;
    int32_t frame_count_;
    uint32_t frame_size_;
    EbSvtIOFormat *frame_buffer_;
    VideoImageFormat image_format_;
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
