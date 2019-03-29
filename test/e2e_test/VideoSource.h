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
    virtual EbErrorType allocate_fream_buffer() {
        // Determine size of each plane
        const size_t luma_8bit_size =
            width_with_padding_ * height_with_padding_;
        size_t chroma8bitSize = 0;
        switch (image_format_) {
        case IMG_FMT_420: chroma8bitSize = luma_8bit_size >> 2; break;
        case IMG_FMT_422: chroma8bitSize = luma_8bit_size >> 1; break;
        case IMG_FMT_444: chroma8bitSize = luma_8bit_size; break;
        }

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
    virtual void init_monitor() {
#ifdef _WIN32
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Input Monitor",
                                  SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED,
                                  width_,
                                  height_,
                                  0);

        renderer = SDL_CreateRenderer(window, -1, 0);
        // TODO: Check more format
        texture = SDL_CreateTexture(renderer,
                                    SDL_PIXELFORMAT_IYUV,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    width_,
                                    height_);
        monitor_buffer = (uint8_t *)SDL_calloc(1, width_ * height_ * 3 / 2);
#endif  // ifdef WIN32
    }
    virtual void draw_frame() {
#ifdef _WIN32
        memcpy(monitor_buffer, frame_buffer_->luma, width_ * height_);
        memcpy(monitor_buffer + width_ * height_,
               frame_buffer_->cb,
               width_ * height_ / 4);
        memcpy(monitor_buffer + width_ * height_ * 5 / 4,
               frame_buffer_->cr,
               width_ * height_ / 4);
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
#ifdef WIN32
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Window *window;
    uint8_t *monitor_buffer;
#endif
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
