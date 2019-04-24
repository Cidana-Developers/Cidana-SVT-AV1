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

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height) (((width) * (height)*3) >> 1)

YuvVideoSource::YuvVideoSource(const std::string &file_name,
                               const VideoImageFormat format,
                               const uint32_t width, const uint32_t height,
                               const uint8_t bit_depth)
    : file_name_(file_name), file_handle_(nullptr) {
    width_with_padding_ = width_ = width;
    height_with_padding_ = height_ = height;
    bit_depth_ = bit_depth;
    frame_count_ = 0;
    current_frame_index_ = -1;
    frame_size_ = 0;
    frame_buffer_ = nullptr;
    image_format_ = format;
}
YuvVideoSource::~YuvVideoSource() {
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }
}

// Prepare stream, and get first frame.
EbErrorType YuvVideoSource::open_source() {
    // Reopen file as necessary
    if (file_handle_ == nullptr) {
        file_handle_ = fopen(file_name_.c_str(), "rb");
    }

    if (file_handle_ == nullptr)
        return EB_ErrorBadParameter;

    // Prepare buffer
    if (EB_ErrorNone != init_frame_buffer()) {
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    // Seek to begin, and get first frame.
    fseek(file_handle_, 0, SEEK_SET);

    current_frame_index_ = -1;

    return EB_ErrorNone;
}

// Prepare stream, and get first frame.
void YuvVideoSource::close_source() {
    if (file_handle_ == nullptr) {
        fclose(file_handle_);
        file_handle_ = nullptr;
    }
    deinit_frame_buffer();
    frame_count_ = 0;
}

// Get next frame.
EbSvtIOFormat *YuvVideoSource::get_next_frame() {
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    ++current_frame_index_;
    return frame_buffer_;
}

uint32_t YuvVideoSource::read_input_frame() {
    uint64_t readSize = 0;
    const uint32_t input_padded_width = width_;
    const uint32_t input_padded_height = height_;
    uint8_t *eb_input_ptr = nullptr;
    uint32_t filled_len = 0;
    if (file_handle_ == nullptr)
        return 0;

    if (feof(file_handle_) != 0) {
        return 0;
    }

    frame_buffer_->y_stride = input_padded_width;
    frame_buffer_->cb_stride = input_padded_width >> 1;
    frame_buffer_->cr_stride = input_padded_width >> 1;

    readSize = (uint64_t)SIZE_OF_ONE_FRAME_IN_BYTES(input_padded_width,
                                                    input_padded_height);

    uint64_t lumaReadSize = (uint64_t)input_padded_width * input_padded_height;

    // Read raw data from file
    eb_input_ptr = frame_buffer_->luma;
    filled_len += (uint32_t)fread(eb_input_ptr, 1, lumaReadSize, file_handle_);

    eb_input_ptr = frame_buffer_->cb;
    filled_len +=
        (uint32_t)fread(eb_input_ptr, 1, lumaReadSize >> 2, file_handle_);

    eb_input_ptr = frame_buffer_->cr;
    filled_len +=
        (uint32_t)fread(eb_input_ptr, 1, lumaReadSize >> 2, file_handle_);

    if (readSize != filled_len) {
        return 0;
    }

    return filled_len;
}
