#ifndef _SVT_TEST_YUV_VIDEO_SOURCE_H_
#define _SVT_TEST_YUV_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"

class YuvVideoSource : public VideoSource {
  public:
    YuvVideoSource(const std::string &file_name, const VideoImageFormat format,
                   const uint32_t width, const uint32_t height,
                   const uint8_t bit_depth);
    ~YuvVideoSource();
    // Prepare stream, and get first frame.
    // Return EB_ErrorBadParameter as open file failed.
    EbErrorType open_source() override;
    // Get next frame.
    EbSvtEncInput *get_next_frame() override;

  private:
    uint32_t read_input_frame();
    std::string file_name_;
    FILE *file_handle_;
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
