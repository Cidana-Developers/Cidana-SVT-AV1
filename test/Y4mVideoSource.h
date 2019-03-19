#ifndef _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#define _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"

typedef enum Y4MColorSpaceType {
    Y4MColor_420JPEG,
    Y4MColor_420PALDV,
    Y4MColor_420,
    Y4MColor_422,
    Y4MColor_444
} Y4MColorSpaceType;

class Y4MVideoSource : public VideoSource {
  public:
    Y4MVideoSource(const std::string &file_name);
    virtual ~Y4MVideoSource();
    // Prepare stream, and get first frame.
    // Return EB_ErrorBadParameter as open file failed.
    virtual EbErrorType to_begin();
    // Get next frame.
    virtual EbErrorType to_next();

  private:
    EbErrorType parse_file_info();
    uint32_t read_input_frames();
    std::string file_name_;
    FILE *file_handle_;
    Y4MColorSpaceType color_space_;
};
#endif  //_SVT_TEST_Y4M_VIDEO_SOURCE_H_
