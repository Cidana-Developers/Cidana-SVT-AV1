/*
* Copyright(c) 2019 Intel Corporation
* SPDX - License - Identifier: BSD - 2 - Clause - Patent
*/
#ifndef _RECON_SINK_H_
#define _RECON_SINK_H_

#include <stdint.h>
#include <memory.h>
#include "VideoFrame.h"

class ReconSink {
  public:
    typedef enum ReconSinkType {
        RECON_SINK_BUFFER,
        RECON_SINK_FILE,
    } ReconSinkType;

    typedef struct ReconMug {
        uint32_t tag;
        uint64_t time_stamp;
        uint32_t mug_size;
        uint32_t filled_size;
        uint8_t* mug_buf;
    } ReconMug;

  public:
    ReconSink(const VideoFrameParam& param) {
        sink_type_ = RECON_SINK_BUFFER;
        video_param_ = param;
        frame_size_ = calculate_frame_size(video_param_);
        frame_count_ = 0;
    }
    virtual ~ReconSink() {
    }
    ReconSinkType get_type() {
        return sink_type_;
    }
    VideoFrameParam get_video_param() {
        return video_param_;
    }
	uint32_t get_frame_count() {
        return frame_count_;
    }
	void set_frame_count(uint32_t count) {
		frame_count_ = count;
	}
    ReconMug* get_empty_mug() {
        ReconMug* new_mug = new ReconMug;
        if (new_mug) {
            memset(new_mug, 0, sizeof(ReconMug));
            new_mug->mug_size = frame_size_;
            new_mug->mug_buf = new uint8_t[frame_size_];
            if (new_mug->mug_buf == nullptr) {
                delete new_mug;
                new_mug = nullptr;
            }
        }
        return new_mug;
    }
    virtual void fill_mug(ReconMug* mug) = 0;
    virtual const ReconMug* take_mug(uint64_t time_stamp) = 0;
    virtual const ReconMug* take_mug_inorder(uint32_t index) = 0;
    virtual void pour_mug(ReconMug* mug) = 0;
	virtual bool is_compelete() = 0;

  protected:
    static uint32_t calculate_frame_size(const VideoFrameParam& param) {
        uint32_t lumaSize = param.width * param.height;
        uint32_t chromaSize = 0;
        switch (param.format) {
        case IMG_FMT_420: chromaSize = lumaSize >> 2; break;
        case IMG_FMT_422: chromaSize = lumaSize >> 1; break;
        case IMG_FMT_444: chromaSize = lumaSize; break;
        case IMG_FMT_420P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize >> 2;
            break;
        case IMG_FMT_422P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize >> 1;
            break;
        case IMG_FMT_444P10_PACKED:
            lumaSize = lumaSize << 1;
            chromaSize = lumaSize;
            break;
        }
        return lumaSize + (2 * chromaSize);
    }

  protected:
    ReconSinkType sink_type_;
    VideoFrameParam video_param_;
    uint32_t frame_size_;
	uint32_t frame_count_;
};

ReconSink* create_recon_sink(const VideoFrameParam& param, const char* file_path);
ReconSink* create_recon_sink(const VideoFrameParam& param);

#endif  // !_RECON_SINK_H_
