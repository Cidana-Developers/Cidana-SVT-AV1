#ifndef _SVT_TEST_YUV_VIDEO_SOURCE_H_
#define _SVT_TEST_YUV_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"

class YuvVideoSource : public VideoSource {
  public:
    YuvVideoSource(const std::string &file_name, const uint32_t width,
                   const uint32_t height, const uint8_t bit_depth);
    virtual ~YuvVideoSource();
    // Prepare stream, and get first frame.
    // Return EB_ErrorBadParameter as open file failed.
    virtual EbErrorType to_begin();
    // Get next frame.
    virtual EbErrorType to_next();
    // Get current frame.
    virtual EbSvtEncInput *get_current_frame();

  private:
    EbErrorType allocate_fream_buffer();
    uint32_t read_input_frames();
    std::string file_name_;
    FILE *file_handle_;
    EbSvtEncInput *frame_buffer_;
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
