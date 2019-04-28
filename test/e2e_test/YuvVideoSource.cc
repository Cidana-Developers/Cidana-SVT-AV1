/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file Y4mVideoSource.h
 *
 * @brief Impelmentation of YuvVideoSource class for reading y4m file.
 *
 * @author Cidana-Ryan
 *
 ******************************************************************************/
#include "YuvVideoSource.h"
using namespace svt_av1_video_source;
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height) (((width) * (height)*3) >> 1)

YuvVideoSource::YuvVideoSource(const std::string &file_name,
                               const VideoColorFormat format,
                               const uint32_t width, const uint32_t height,
                               const uint8_t bit_depth)
    : VideoFileSource(file_name, format, width, height, bit_depth, false) {
#ifdef ENABLE_DEBUG_MONITOR
    monitor = nullptr;
#endif
}
YuvVideoSource::~YuvVideoSource() {
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }
#ifdef ENABLE_DEBUG_MONITOR
    if (monitor != nullptr)
        delete monitor;
#endif
}

// Prepare stream, and get first frame.
EbErrorType YuvVideoSource::open_source(const uint32_t init_pos,
                                        const uint32_t frame_count) {
    std::string full_path = get_vector_path() + "/" + file_name_.c_str();
    if (file_handle_ == nullptr) {
        file_handle_ = fopen(full_path.c_str(), "rb");
    }

    if (file_handle_ == nullptr)
        return EB_ErrorBadParameter;

    // Prepare buffer
    if (EB_ErrorNone != init_frame_buffer()) {
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    // Get file length
    fseek(file_handle_, 0, SEEK_END);
    file_length_ = ftell(file_handle_);

    // Seek to begin
    fseek(file_handle_, 0, SEEK_SET);

    frame_length_ = width_ * height_;

    // Calculate frame length
    switch (image_format_) {
    case IMG_FMT_420P10_PACKED:
    case IMG_FMT_420: {
        frame_length_ = frame_length_ * 3 / 2 * (bit_depth_ > 8 ? 2 : 1);
    } break;
    case IMG_FMT_422P10_PACKED:
    case IMG_FMT_422: {
        frame_length_ = frame_length_ * 2 * (bit_depth_ > 8 ? 2 : 1);
    } break;
    case IMG_FMT_444P10_PACKED:
    case IMG_FMT_444: {
        frame_length_ = frame_length_ * 3 * (bit_depth_ > 8 ? 2 : 1);
    } break;
    default: break;
    }

    // Calculate frame count
    file_frames_ = file_length_ / frame_length_;

    if (file_frames_ <= init_pos || init_pos + frame_count >= file_frames_) {
        printf(
            "setup of initial position(%u) and output frame count(%u) is out "
            "of bound!\n",
            init_pos,
            frame_count);
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }
    init_pos_ = init_pos;
    if (frame_count == 0)
        frame_count_ = file_frames_ - init_pos_;
    else
        frame_count_ = frame_count;
    fseek(file_handle_, init_pos_ * frame_length_, SEEK_SET);

    current_frame_index_ = -1;
#ifdef ENABLE_DEBUG_MONITOR
    if (monitor == nullptr) {
        monitor = new VideoMonitor(
            width_with_padding_,
            height_with_padding_,
            (bit_depth_ > 8) ? width_with_padding_ * 2 : width_with_padding_,
            bit_depth_,
            svt_compressed_2bit_plane_,
            "YUV Source");
    }
#endif
    return EB_ErrorNone;
}
// Close video source.
void YuvVideoSource::close_source() {
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }
    deinit_frame_buffer();
    init_pos_ = 0;
    frame_count_ = 0;
    file_frames_ = 0;
}

EbSvtIOFormat *YuvVideoSource::get_frame_by_index(const uint32_t index) {
    if (index > frame_count_) {
        return nullptr;
    }
    // Seek to frame by index
    fseek(file_handle_, (init_pos_ + index) * frame_length_, SEEK_SET);
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    current_frame_index_ = index;
#ifdef ENABLE_DEBUG_MONITOR
    if (monitor != nullptr) {
        monitor->draw_frame(
            frame_buffer_->luma, frame_buffer_->cb, frame_buffer_->cr);
    }
#endif
    return frame_buffer_;
}

// Get next frame.
EbSvtIOFormat *YuvVideoSource::get_next_frame() {
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    ++current_frame_index_;
#ifdef ENABLE_DEBUG_MONITOR
    if (monitor != nullptr) {
        monitor->draw_frame(
            frame_buffer_->luma, frame_buffer_->cb, frame_buffer_->cr);
    }
#endif
    return frame_buffer_;
}
