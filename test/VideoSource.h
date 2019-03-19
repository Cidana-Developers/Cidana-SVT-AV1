#ifndef _SVT_TEST_VIDEO_SOURCE_H_
#define _SVT_TEST_VIDEO_SOURCE_H_
#include "EbApi.h"
// Abstract base class for test video source
class VideoSource {
  public:
    virtual ~VideoSource(){};
    // Prepare stream, and get first frame.
    virtual EbErrorType to_begin() = 0;
    // Get next frame.
    virtual EbErrorType to_next() = 0;
    // Get current frame.
    virtual EbSvtEncInput *get_current_frame() = 0;
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

  protected:
    uint32_t width_;
    uint32_t height_;
    uint32_t bit_depth_;
    uint32_t frame_count_;
    uint32_t frame_size_;
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
