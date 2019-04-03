
#include "VideoMonitor.h"

uint32_t VideoMonitor::ref_cout = 0;
VideoMonitor::VideoMonitor(const uint32_t width, const uint32_t height,
                           const uint8_t bit_depth,
                           const bool packed_ten_bit_mode)
    : width_(width),
      height_(height),
      bit_depth_(bit_depth),
      packed_ten_bit_mode_(packed_ten_bit_mode) {
#ifdef ENABLE_DEBUG_MONITOR
    printf("init_monitor:%d:%d\r\n", width, height);
    if (ref_cout == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            printf("SDL_Init Error!\r\n");
            return;
        }
    }
    ref_cout++;

    window = SDL_CreateWindow("Input Monitor",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width,
                              height,
                              SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == nullptr) {
        printf("SDL_CreateWindow Error!\r\n");
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == nullptr) {
        printf("SDL_CreateRenderer Error!\r\n");
        return;
    }
    // TODO: Check more format
    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_IYUV,
                                SDL_TEXTUREACCESS_STREAMING,
                                width,
                                height);
    if (texture == nullptr) {
        printf("SDL_CreateTexture Error!\r\n");
        return;
    }
    monitor_buffer = (uint8_t *)SDL_calloc(1, width * height * 3);

#ifndef _WIN32
    SDL_Event e;
    bool quit = false;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN) {
                quit = true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                quit = true;
            }
        }
    }
#endif
#endif
}

VideoMonitor::~VideoMonitor() {
#ifdef ENABLE_DEBUG_MONITOR
    ref_cout--;
    if (ref_cout == 0)
        SDL_Quit();
#endif
}
void VideoMonitor::draw_frame(const uint8_t *luma, const uint8_t *cb,
                              const uint8_t *cr) {
#ifdef ENABLE_DEBUG_MONITOR
    const unsigned int luma_len = width_ * height_;
    if (bit_depth_ == 8 || ((bit_depth_ == 10) && !packed_ten_bit_mode_)) {
        memcpy(monitor_buffer, luma, luma_len);
        memcpy(monitor_buffer + luma_len, cb, luma_len / 4);
        memcpy(monitor_buffer + luma_len * 5 / 4, cr, luma_len / 4);
    } else if (bit_depth_ == 10 && packed_ten_bit_mode_) {
        for (int i = 0; i < luma_len; i++) {
            uint16_t w = ((uint16_t *)luma)[i];
            monitor_buffer[i] = w >> 2 & 0xFF;
        }
        for (int i = 0; i < (luma_len >> 2); i++) {
            uint16_t w = ((uint16_t *)cb)[i];
            monitor_buffer[luma_len + i] = w >> 2 & 0xFF;
        }
        for (int i = 0; i < (luma_len >> 2); i++) {
            uint16_t w = ((uint16_t *)cr)[i];
            monitor_buffer[(luma_len * 5 / 4) + i] = w >> 2 & 0xFF;
        }
    }
    SDL_UpdateTexture(texture, NULL, monitor_buffer, width_);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
// usleep(1000*1000);
#endif
}