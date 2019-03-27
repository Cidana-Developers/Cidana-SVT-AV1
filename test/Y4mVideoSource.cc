#include "Y4mVideoSource.h"
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height) (((width) * (height)*3) >> 1)

Y4MVideoSource::Y4MVideoSource(const std::string& file_name)
    : file_name_(file_name), file_handle_(nullptr) {
    width_ = 0;
    height_ = 0;
    bit_depth_ = 0;
    frame_count_ = -1;
    frame_size_ = 0;
    frame_buffer_ = nullptr;
    image_format_ = IMG_FMT_420;
}

Y4MVideoSource::~Y4MVideoSource() {
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
EbErrorType Y4MVideoSource::open_source() {
    EbErrorType return_error = EB_ErrorNone;
    // Reopen file as necessary
    if (file_handle_ == nullptr) {
        file_handle_ = fopen(file_name_.c_str(), "rb");
    }

    if (file_handle_ == nullptr)
        return EB_ErrorBadParameter;

    // Seek to begin
    fseek(file_handle_, 0, SEEK_SET);

    // Get file info before prepare buffer
    return_error = parse_file_info();
    if (return_error != EB_ErrorNone) {
        return return_error;
    }

    // Prepare buffer
    if (EB_ErrorNone != allocate_fream_buffer()) {
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    frame_count_ = -1;
    init_monitor();

    return EB_ErrorNone;
}

// Get next frame.
EbSvtEncInput* Y4MVideoSource::get_next_frame() {
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    ++frame_count_;
    draw_frame();
    return frame_buffer_;
}

#define SKIP_TAG                                      \
    {                                                 \
        char tmp;                                     \
        do {                                          \
            if (1 != fread(&tmp, 1, 1, file_handle_)) \
                break;                                \
        } while ((tmp != 0xA) && (tmp != ' '));       \
    }

EbErrorType Y4MVideoSource::parse_file_info() {
    char buffer[10] = {0};
    char first_char;
    if (file_handle_ == nullptr)
        return EB_ErrorBadParameter;

    // Check file header "YUV4MPEG2 "
    if (9 != fread(buffer, 1, 9, file_handle_)) {
        return EB_ErrorBadParameter;
    }

    if (0 != strcmp("YUV4MPEG2", buffer)) {
        return EB_ErrorBadParameter;
    }

    do {
        if (1 != fread(&first_char, 1, 1, file_handle_))
            break;
        if (first_char == ' ')
            continue;

        switch (first_char) {
        case 'W':  // Width
        {
            fscanf(file_handle_, "%d ", &width_);
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'H':  // Height
        {
            fscanf(file_handle_, "%d ", &height_);
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'F':  // Frame rate
        {
            uint32_t tmp1, tmp2;
            fscanf(file_handle_, "%d:%d ", &tmp1, &tmp2);
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'I':  // Interlacing
        {
            char tmp;
            fscanf(file_handle_, "%c ", &tmp);
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'A':  // Pixel aspect ratio.
        {
            uint32_t tmp1, tmp2;
            fscanf(file_handle_, "%d:%d ", &tmp1, &tmp2);
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'C':  // Color space
        {
            SKIP_TAG;
            // TODO:Support other type, support 420 currently.
            image_format_ = IMG_FMT_420;
            fseek(file_handle_, -1, SEEK_CUR);

        } break;
        case 'X':  // Comment
        {
            SKIP_TAG;
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        default: break;
        }
    } while (!feof(file_handle_) && (first_char != 0xA));
    return EB_ErrorNone;
}

uint32_t Y4MVideoSource::read_input_frame() {
    uint64_t readSize = 0;
    const uint32_t input_padded_width = width_;
    const uint32_t input_padded_height = height_;
    uint8_t* eb_input_ptr = nullptr;
    uint32_t filled_len = 0;
    char frame_header[6] = {0};
    if (file_handle_ == nullptr)
        return 0;

    if (feof(file_handle_) != 0)
        return 0;

    int nread = fread(frame_header, 1, 6, file_handle_);
    // if (6 != fread(frame_header, 1, 6, file_handle_))
    // return 0;

    // Check frame header
    if (!((strncmp("FRAME", frame_header, 5) == 0) && frame_header[5] == 0xA))
        return 0;

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
