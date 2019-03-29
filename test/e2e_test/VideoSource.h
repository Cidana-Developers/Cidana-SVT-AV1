#ifndef _SVT_TEST_VIDEO_SOURCE_H_
#define _SVT_TEST_VIDEO_SOURCE_H_
#include "EbSvtAv1Enc.h"
#ifdef _WIN32
#include "SDL.h"
#endif

typedef enum VideoImageFormat {
    IMG_FMT_420,
    IMG_FMT_422,
    IMG_FMT_444,
    IMG_FMT_420P10_PACKED,
    IMG_FMT_422P10_PACKED,
    IMG_FMT_444P10_PACKED
} VideoImageFormat;

// Abstract base class for test video source
class VideoSource {
  public:
    virtual ~VideoSource() {
        deinit_monitor();
        deinit_frame_buffer();
    };
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
    // Get video width
    virtual uint32_t get_width_with_padding() {
        return width_with_padding_;
    };
    // Get video height
    virtual uint32_t get_height_with_padding() {
        return height_with_padding_;
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
        if (frame_buffer_->lumaExt != nullptr) {
            free(frame_buffer_->lumaExt);
            frame_buffer_->lumaExt = nullptr;
        }
        if (frame_buffer_->cbExt != nullptr) {
            free(frame_buffer_->cbExt);
            frame_buffer_->cbExt = nullptr;
        }
        if (frame_buffer_->crExt != nullptr) {
            free(frame_buffer_->crExt);
            frame_buffer_->crExt = nullptr;
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

        if (is_ten_bit_mode() && packed_ten_bit_mode) {
            luma_size *= 2;
            chroma_size *= 2;
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

        frame_buffer_->yStride = luma_size;
        frame_buffer_->cbStride = chroma_size;
        frame_buffer_->crStride = chroma_size;

        if (is_ten_bit_mode() && !packed_ten_bit_mode) {
            frame_buffer_->lumaExt = (uint8_t *)malloc(luma_size);
            if (!frame_buffer_->lumaExt) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cbExt = (uint8_t *)malloc(chroma_size);
            if (!frame_buffer_->cbExt) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }

            frame_buffer_->cr = (uint8_t *)malloc(chroma_size);
            if (!frame_buffer_->crExt) {
                deinit_frame_buffer();
                return EB_ErrorInsufficientResources;
            }
        } else {
            frame_buffer_->lumaExt = nullptr;
            frame_buffer_->cbExt = nullptr;
            frame_buffer_->crExt = nullptr;
        }

        return EB_ErrorNone;
    }
    virtual void init_monitor() {
#ifdef _WIN32
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Input Monitor",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  width_with_padding_,
                                  height_with_padding_,
                                  0);

        renderer = SDL_CreateRenderer(window, -1, 0);
        // TODO: Check more format
        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_IYUV,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width_with_padding_,
                                    height_with_padding_);
        monitor_buffer = (uint8_t *)SDL_calloc(
            1, width_with_padding_ * height_with_padding_ * 3);
#endif  // ifdef WIN32
    }
    virtual void draw_frame() {
#ifdef _WIN32
        const unsigned int luma_len =
            width_with_padding_ * height_with_padding_;
        if (bit_depth_ == 8 || ((bit_depth_ == 10) && !packed_ten_bit_mode)) {
            memcpy(monitor_buffer, frame_buffer_->luma, luma_len);
            memcpy(monitor_buffer + luma_len, frame_buffer_->cb, luma_len / 4);
            memcpy(monitor_buffer + luma_len * 5 / 4,
                   frame_buffer_->cr,
                   luma_len / 4);
        } else if (bit_depth_ == 10 && packed_ten_bit_mode) {
            for (int i = 0; i < luma_len; i++) {
                uint16_t w = ((uint16_t *)frame_buffer_->luma)[i];
                monitor_buffer[i] = w >> 2 & 0xFF;
            }
            for (int i = 0; i < (luma_len >> 2); i++) {
                uint16_t w = ((uint16_t *)frame_buffer_->cb)[i];
                monitor_buffer[luma_len + i] = w >> 2 & 0xFF;
            }
            for (int i = 0; i < (luma_len >> 2); i++) {
                uint16_t w = ((uint16_t *)frame_buffer_->cr)[i];
                monitor_buffer[(luma_len * 5 / 4) + i] = w >> 2 & 0xFF;
            }
        }
        SDL_UpdateTexture(texture, NULL, monitor_buffer, width_);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
#endif
    }
    virtual void deinit_monitor() {
#ifdef _WIN32
        SDL_Quit();
#endif
    }
    uint32_t width_;
    uint32_t height_;
    uint32_t width_with_padding_;
    uint32_t height_with_padding_;
    uint32_t bit_depth_;
    int32_t frame_count_;
    uint32_t frame_size_;
    EbSvtIOFormat *frame_buffer_;
    VideoImageFormat image_format_;
    bool packed_ten_bit_mode;
#ifdef WIN32
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Window *window;
    uint8_t *monitor_buffer;
#endif
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
