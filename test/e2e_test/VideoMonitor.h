#ifndef _VIDEO_MONITOR_
#define _VIDEO_MONITOR_

#include "SDL.h"
#include "unistd.h"
#include "pthread.h"
#include "EbSvtAv1Enc.h"

class VideoMonitor {
  public:
    VideoMonitor(const uint32_t width, const uint32_t height,
                 const uint8_t bit_depth, const bool packed_ten_bit_mode);

    ~VideoMonitor();
    void draw_frame(const uint8_t *luma, const uint8_t *cb, const uint8_t *cr);

  private:
    static uint32_t ref_cout;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Window *window;
    uint8_t *monitor_buffer;
    const uint32_t width_;
    const uint32_t height_;
    const uint8_t bit_depth_;
    const bool packed_ten_bit_mode_;
};
#endif  //_VIDEO_MONITOR_