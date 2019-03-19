#include "YuvVideosource.h"

#define LEFT_INPUT_PADDING 0
#define RIGHT_INPUT_PADDING 0
#define TOP_INPUT_PADDING 0
#define BOTTOM_INPUT_PADDING 0

#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height) (((width) * (height)*3) >> 1)

YuvVideoSource::YuvVideoSource(const std::string &file_name,
                               const uint32_t width, const uint32_t height,
                               const uint8_t bit_depth)
    : file_name_(file_name), file_handle_(nullptr), frame_buffer_(nullptr) {
    width_ = width;
    height_ = height;
    bit_depth_ = bit_depth;
    frame_count_ = 0;
    frame_size_ = 0;
}
YuvVideoSource::~YuvVideoSource() {
    if (frame_buffer_ != nullptr) {
        if (frame_buffer_->luma != nullptr)
            free(frame_buffer_->luma);
        if (frame_buffer_->cb != nullptr)
            free(frame_buffer_->cb);
        if (frame_buffer_->cr != nullptr)
            free(frame_buffer_->cr);
        free(frame_buffer_);
    }
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
    }
}

// Prepare stream, and get first frame.
EbErrorType YuvVideoSource::to_begin() {
    // Reopen file as necessary
    if (file_handle_ == nullptr) {
        file_handle_ = fopen(file_name_.c_str(), "rb");
    }

    if (file_handle_ == nullptr)
        return EB_ErrorBadParameter;

    // Prepare buffer
    if (EB_ErrorNone != allocate_fream_buffer()) {
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    // Seek to begin, and get first frame.
    fseek(file_handle_, 0, SEEK_SET);

    frame_count_ = 0;
    frame_size_ = read_input_frames();
    if (frame_size_ == 0)
        return EB_ErrorInsufficientResources;

    return EB_ErrorNone;
}

// Get next frame.
EbErrorType YuvVideoSource::to_next() {
    frame_size_ = read_input_frames();
    if (frame_size_ == 0)
        return EB_ErrorInsufficientResources;
    ++frame_count_;
    return EB_ErrorNone;
}

EbSvtEncInput *YuvVideoSource::get_current_frame() {
    return frame_buffer_;
}

EbErrorType YuvVideoSource::allocate_fream_buffer() {
    // Determine size of each plane
    const size_t luma_8bit_size = width_ * height_;
    const size_t chroma8bitSize = luma_8bit_size >> 2;

    // Determine
    if (frame_buffer_ == nullptr)
        frame_buffer_ = (EbSvtEncInput *)malloc(sizeof(EbSvtEncInput));
    else
        return EB_ErrorNone;

    frame_buffer_->luma = (uint8_t *)malloc(luma_8bit_size);
    if (!frame_buffer_->luma) {
        free(frame_buffer_);
        frame_buffer_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    frame_buffer_->cb = (uint8_t *)malloc(chroma8bitSize);
    if (!frame_buffer_->cb) {
        free(frame_buffer_->luma);
        free(frame_buffer_);
        frame_buffer_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    frame_buffer_->cr = (uint8_t *)malloc(chroma8bitSize);
    if (!frame_buffer_->cr) {
        free(frame_buffer_->cb);
        free(frame_buffer_->luma);
        free(frame_buffer_);
        frame_buffer_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    return EB_ErrorNone;
}

uint32_t YuvVideoSource::read_input_frames() {
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

    frame_buffer_->yStride = input_padded_width;
    frame_buffer_->cbStride = input_padded_width >> 1;
    frame_buffer_->crStride = input_padded_width >> 1;

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
