#include "Y4mVideoSource.h"
#define SIZE_OF_ONE_FRAME_IN_BYTES(width, height) (((width) * (height)*3) >> 1)

Y4MVideoSource::Y4MVideoSource(const std::string& file_name,
                               const VideoImageFormat format,
                               const uint32_t width, const uint32_t height,
                               const uint8_t bit_depth)
    : file_name_(file_name), file_handle_(nullptr) {
    width_ = width;
    width_with_padding_ = width_;
    if (width_ % 8 != 0)
        width_with_padding_ += +(8 - width_ % 8);
    height_ = height;
    height_with_padding_ = height_;
    if (height_ % 8 != 0)
        height_with_padding_ += (8 - height_ % 8);
    bit_depth_ = bit_depth;
    frame_count_ = 0;
    current_frame_index_ = -1;
    frame_size_ = 0;
    frame_buffer_ = nullptr;
    image_format_ = format;
    packed_ten_bit_mode = true;
    monitor = nullptr;
}

Y4MVideoSource::~Y4MVideoSource() {
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
    }
    if (monitor != nullptr)
        delete monitor;
}

// Prepare stream, and get first frame.
EbErrorType Y4MVideoSource::open_source() {
    EbErrorType return_error = EB_ErrorNone;
    if (file_handle_ != nullptr)
        return EB_ErrorNone;

    file_handle_ = fopen(file_name_.c_str(), "rb");
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
    if (EB_ErrorNone != init_frame_buffer()) {
        fclose(file_handle_);
        file_handle_ = nullptr;
        return EB_ErrorInsufficientResources;
    }

    current_frame_index_ = -1;
    monitor = new VideoMonitor(width_with_padding_,
                               height_with_padding_,
                               bit_depth_,
                               packed_ten_bit_mode);

    return EB_ErrorNone;
}

EbSvtIOFormat* Y4MVideoSource::get_frame_by_index(const uint32_t index) {
    if (index > frame_count_) {
        return nullptr;
    }
    // Seek to index frame
    fseek(file_handle_, index * frame_length_ + header_length_, SEEK_SET);
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    current_frame_index_ = index;
    monitor->draw_frame(
        frame_buffer_->luma, frame_buffer_->cb, frame_buffer_->cr);
    return frame_buffer_;
}

// Get next frame.
EbSvtIOFormat* Y4MVideoSource::get_next_frame() {
    frame_size_ = read_input_frame();
    if (frame_size_ == 0)
        return nullptr;
    ++current_frame_index_;
    monitor->draw_frame(
        frame_buffer_->luma, frame_buffer_->cb, frame_buffer_->cr);
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

    // Get file length
    fseek(file_handle_, 0, SEEK_END);
    file_length_ = ftell(file_handle_);

    // Seek to begin
    fseek(file_handle_, 0, SEEK_SET);

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
            width_with_padding_ = width_;
            if (width_ % 8 != 0)
                width_with_padding_ += +(8 - width_ % 8);
        } break;
        case 'H':  // Height
        {
            fscanf(file_handle_, "%d ", &height_);
            fseek(file_handle_, -1, SEEK_CUR);
            height_with_padding_ = height_;
            if (height_ % 8 != 0)
                height_with_padding_ += (8 - height_ % 8);
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
            char line[80] = {0};
            int line_len = 0;
            char c;
            do {
                line[line_len++] = c = fgetc(file_handle_);
            } while (c != ' ' && c != 0x0A && line_len < 80);
            if (strncmp("420p10", line, 6) == 0) {
                bit_depth_ = 10;
                image_format_ = IMG_FMT_420P10_PACKED;
            } else if (strncmp("422p10", line, 6) == 0) {
                bit_depth_ = 10;
                image_format_ = IMG_FMT_422P10_PACKED;
            } else if (strncmp("444p10", line, 6) == 0) {
                bit_depth_ = 10;
                image_format_ = IMG_FMT_444P10_PACKED;
            } else if (strncmp("420jpeg", line, 7) == 0 ||
                       strncmp("420", line, 3) == 0) {
                bit_depth_ = 8;
                image_format_ = IMG_FMT_420;
            } else if (strncmp("422", line, 3) == 0) {
                bit_depth_ = 8;
                image_format_ = IMG_FMT_422;
            } else if (strncmp("444", line, 3) == 0) {
                bit_depth_ = 8;
                image_format_ = IMG_FMT_444;
            } else {
                printf("chroma format not supported\n");
                return EB_ErrorBadParameter;
            }
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

    // Get header lenght
    header_length_ = ftell(file_handle_);
    frame_length_ = width_ * height_;

    // Calculate frame length
    switch (image_format_) {
    case IMG_FMT_420P10_PACKED: {
        frame_length_ *= 2;
    }
    case IMG_FMT_420: {
        frame_length_ = frame_length_ * 3 / 2;
    } break;
    case IMG_FMT_422P10_PACKED: frame_length_ *= 2;
    case IMG_FMT_422: {
        frame_length_ = frame_length_ * 2;
    } break;
    case IMG_FMT_444P10_PACKED: frame_length_ *= 2;
    case IMG_FMT_444: {
        frame_length_ = frame_length_ * 3;
    } break;
    default: break;
    }
    frame_length_ += 6;  // FRAME header

    // Calculate frame count
    frame_count_ = (file_length_ - header_length_) / frame_length_;

    printf("File len:%d; frame w:%d, h:%d, len:%d; frame count:%d\r\n",
           file_length_,
           width_,
           height_,
           frame_length_,
           frame_count_);

    return EB_ErrorNone;
}

uint32_t Y4MVideoSource::read_input_frame() {
    uint64_t readSize = 0;
    uint8_t* eb_input_ptr = nullptr;
    uint32_t filled_len = 0;
    char frame_header[6] = {0};
    if (file_handle_ == nullptr) {
        printf("Error file handle\r\n");
        return 0;
    }

    if (feof(file_handle_) != 0) {
        printf("Reach file end\r\n");
        return 0;
    }

    if (6 != fread(frame_header, 1, 6, file_handle_))
        return 0;

    // Check frame header
    if (!((strncmp("FRAME", frame_header, 5) == 0) && frame_header[5] == 0xA)) {
        printf("Read frame error\n");
        return 0;
    }

    int chroma_down_size = 2;
    int width_downsize = 1;
    int height_downsize = 1;
    int pixel_byte_size = 1;

    switch (image_format_) {
    case IMG_FMT_420: {
        chroma_down_size = 2;
        width_downsize = 1;
        height_downsize = 1;
        pixel_byte_size = 1;
        break;
    }
    case IMG_FMT_422: {
        chroma_down_size = 1;
        width_downsize = 1;
        height_downsize = 0;
        pixel_byte_size = 1;
        break;
    }
    case IMG_FMT_444: {
        chroma_down_size = 0;
        width_downsize = 0;
        height_downsize = 0;
        pixel_byte_size = 1;
        break;
    }
    case IMG_FMT_420P10_PACKED: {
        chroma_down_size = 2;
        width_downsize = 1;
        height_downsize = 1;
        pixel_byte_size = 2;
        break;
    }
    case IMG_FMT_422P10_PACKED: {
        chroma_down_size = 1;
        width_downsize = 1;
        height_downsize = 0;
        pixel_byte_size = 2;
        break;
    }
    case IMG_FMT_444P10_PACKED: {
        chroma_down_size = 0;
        width_downsize = 0;
        height_downsize = 0;
        pixel_byte_size = 2;
        break;
    }
    default: break;
    }

    frame_buffer_->yStride = width_with_padding_ * pixel_byte_size;
    frame_buffer_->cbStride =
        (width_with_padding_ >> width_downsize) * pixel_byte_size;
    frame_buffer_->crStride =
        (width_with_padding_ >> width_downsize) * pixel_byte_size;

    // Read raw data from file

    const unsigned int bottom_padding = height_with_padding_ - height_;
    const unsigned int righ_padding = width_with_padding_ - width_;
    unsigned int read_len = 0;
    unsigned int i;
    // Y
    eb_input_ptr = frame_buffer_->luma;
    for (i = 0; i < height_; ++i) {
        read_len = (uint32_t)fread(
            eb_input_ptr, 1, width_ * pixel_byte_size, file_handle_);
        if (read_len != width_ * pixel_byte_size)
            return 0;  // read file error.

        memset(eb_input_ptr + width_ * pixel_byte_size,
               0x14,
               righ_padding * pixel_byte_size);
        eb_input_ptr += frame_buffer_->yStride;
        filled_len += frame_buffer_->yStride;
    }
    for (i = 0; i < bottom_padding; ++i) {
        memset(eb_input_ptr, 0x14, width_with_padding_ * pixel_byte_size);
        eb_input_ptr += frame_buffer_->yStride;
        filled_len += frame_buffer_->yStride;
    }
    // Cb
    eb_input_ptr = frame_buffer_->cb;
    for (i = 0; i < (height_ >> height_downsize); ++i) {
        read_len = (uint32_t)fread(eb_input_ptr,
                                   1,
                                   (width_ >> width_downsize) * pixel_byte_size,
                                   file_handle_);
        if (read_len != (width_ >> width_downsize) * pixel_byte_size)
            return 0;  // read file error.

        memset(eb_input_ptr + (width_ >> width_downsize) * pixel_byte_size,
               0x14,
               (righ_padding >> width_downsize) * pixel_byte_size);
        eb_input_ptr += frame_buffer_->cbStride;
        filled_len += frame_buffer_->cbStride;
    }
    for (i = 0; i < (bottom_padding >> height_downsize); ++i) {
        memset(eb_input_ptr,
               0x14,
               (width_with_padding_ >> width_downsize) * pixel_byte_size);
        eb_input_ptr += frame_buffer_->cbStride;
        filled_len += frame_buffer_->cbStride;
    }

    // Cr
    eb_input_ptr = frame_buffer_->cr;

    for (i = 0; i < (height_ >> height_downsize); ++i) {
        read_len = (uint32_t)fread(eb_input_ptr,
                                   1,
                                   (width_ >> width_downsize) * pixel_byte_size,
                                   file_handle_);
        if (read_len != (width_ >> width_downsize) * pixel_byte_size)
            return 0;  // read file error.

        memset(eb_input_ptr + (width_ >> width_downsize) * pixel_byte_size,
               0x14,
               (righ_padding >> width_downsize) * pixel_byte_size);
        eb_input_ptr += frame_buffer_->cbStride;
        filled_len += frame_buffer_->cbStride;
    }
    for (i = 0; i < (bottom_padding >> height_downsize); ++i) {
        memset(eb_input_ptr,
               0x14,
               (width_with_padding_ >> width_downsize) * pixel_byte_size);
        eb_input_ptr += frame_buffer_->cbStride;
        filled_len += frame_buffer_->cbStride;
    }

    //     printf("Target:[%dx%d:%d],Read[%d]\r\n",
    //            width_with_padding_,
    //            height_with_padding_,
    //            (width_with_padding_ * height_with_padding_) +
    //                2 * ((width_with_padding_ * height_with_padding_) >>
    //                     chroma_down_size),
    //            filled_len);

    return filled_len;
}
