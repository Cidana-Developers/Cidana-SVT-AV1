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
	// Close stream.
	virtual void close_source() override;
	// Get next frame.
    EbSvtIOFormat *get_next_frame() override;
    // Get frame ny index.
    EbSvtIOFormat *get_frame_by_index(const uint32_t index) override { return nullptr; };
  private: 
    uint32_t read_input_frame();
    std::string file_name_;
    FILE *file_handle_;
};

#endif  //_SVT_TEST_VIDEO_SOURCE_H_
