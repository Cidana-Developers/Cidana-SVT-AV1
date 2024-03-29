/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file Y4mVideoSource.h
 *
 * @brief Impelmentation of Y4mVideoSource class for reading y4m file.
 *
 * @author Cidana-Ryan
 *
 ******************************************************************************/
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include "Y4mVideoSource.h"
using namespace svt_av1_video_source;

Y4MVideoSource::Y4MVideoSource(const std::string& file_name,
                               const VideoColorFormat format,
                               const uint32_t width, const uint32_t height,
                               const uint8_t bit_depth,
                               const bool use_compressed_2bit_plane_output)
    : VideoFileSource(file_name, format, width, height, bit_depth,
                      use_compressed_2bit_plane_output) {
    frame_length_ = 0;
    header_length_ = 0;
#ifdef ENABLE_DEBUG_MONITOR
    monitor = nullptr;
#endif
}

Y4MVideoSource::~Y4MVideoSource() {
    if (file_handle_ != nullptr) {
        fclose(file_handle_);
    }
#ifdef ENABLE_DEBUG_MONITOR
    if (monitor != nullptr)
        delete monitor;
#endif
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
            if (fscanf(file_handle_, "%d ", &width_) <= 0)
                return EB_ErrorUndefined;
            fseek(file_handle_, -1, SEEK_CUR);
            width_with_padding_ = width_;
            if (width_ % 8 != 0)
                width_with_padding_ += 8 - width_ % 8;
        } break;
        case 'H':  // Height
        {
            if (fscanf(file_handle_, "%d ", &height_) <= 0)
                return EB_ErrorUndefined;
            fseek(file_handle_, -1, SEEK_CUR);
            height_with_padding_ = height_;
            if (height_ % 8 != 0)
                height_with_padding_ += (8 - height_ % 8);
        } break;
        case 'F':  // Frame rate
        {
            uint32_t tmp1, tmp2;
            if (fscanf(file_handle_, "%d:%d ", &tmp1, &tmp2) <= 0)
                return EB_ErrorUndefined;
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'I':  // Interlacing
        {
            char tmp;
            if (fscanf(file_handle_, "%c ", &tmp) <= 0)
                return EB_ErrorUndefined;
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'A':  // Pixel aspect ratio.
        {
            uint32_t tmp1, tmp2;
            if (fscanf(file_handle_, "%d:%d ", &tmp1, &tmp2) <= 0)
                return EB_ErrorUndefined;
            fseek(file_handle_, -1, SEEK_CUR);
        } break;
        case 'C':  // Color space
        {
            char line[80] = {0};
            int line_len = 0;
            char c;
            do {
                line[line_len++] = c = (char)fgetc(file_handle_);
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
    frame_length_ += 6;  // FRAME header

    // Calculate frame count
    file_frames_ = (file_length_ - header_length_) / frame_length_;

    printf("File len:%d; frame w:%d, h:%d, len:%d; frame count:%d\r\n",
           file_length_,
           width_,
           height_,
           frame_length_,
           file_frames_);

    return EB_ErrorNone;
}

uint32_t Y4MVideoSource::read_input_frame() {
    char frame_header[6] = {0};
    if (file_handle_ == nullptr) {
        printf("Error file handle\r\n");
        return 0;
    }

    if (feof(file_handle_) != 0) {
        printf("Reach file end\r\n");
        return 0;
    }

    if (6 != fread(frame_header, 1, 6, file_handle_)) {
        printf("can not found frame header\r\n");
        return 0;
    }

    // Check frame header
    if (!((strncmp("FRAME", frame_header, 5) == 0) && frame_header[5] == 0xA)) {
        printf("Read frame error\n");
        return 0;
    }
    return VideoFileSource::read_input_frame();
}

EbErrorType Y4MVideoSource::seek_to_frame(const uint32_t index) {
    if (file_handle_ == nullptr) {
        printf("Error file handle\r\n");
        return EB_ErrorInsufficientResources;
    }
    uint32_t real_index = init_pos_ + index;
    if (real_index >= file_frames_)
        real_index = real_index % file_frames_;
    if (fseek(file_handle_,
              real_index * frame_length_ + header_length_,
              SEEK_SET) != 0)
        return EB_ErrorInsufficientResources;
    return EB_ErrorNone;
}
