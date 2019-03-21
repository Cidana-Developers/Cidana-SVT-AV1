#ifndef _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#define _SVT_TEST_Y4M_VIDEO_SOURCE_H_
#include <cstdio>
#include <string>
#include "VideoSource.h"

class Y4MVideoSource : public VideoSource {
  public:
    Y4MVideoSource(const std::string &file_name);
    ~Y4MVideoSource();
    // Prepare stream, and get first frame.
    // Return EB_ErrorBadParameter as open file failed.
    EbErrorType open_source() override;
    // Get next frame.
    EbSvtEncInput *get_next_frame() override;

  private:
    EbErrorType parse_file_info();
    uint32_t read_input_frame();
    std::string file_name_;
    FILE *file_handle_;
};
#endif  //_SVT_TEST_Y4M_VIDEO_SOURCE_H_
