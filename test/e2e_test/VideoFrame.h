/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file VideoFrame.h
 *
 * @brief Defines video frame types and structure
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/
#ifndef _SVT_TEST_VIDEO_FRAME_H_
#define _SVT_TEST_VIDEO_FRAME_H_

#include <memory.h>

/** VideoImageFormat defines the format of YUV video */
typedef enum VideoImageFormat {
    IMG_FMT_YV12,
    IMG_FMT_420 = IMG_FMT_YV12,
    IMG_FMT_422,
    IMG_FMT_444,
    IMG_FMT_420P10_PACKED,
    IMG_FMT_422P10_PACKED,
    IMG_FMT_444P10_PACKED,
    IMG_FMT_NV12,
    IMG_FMT_YV12_CUSTOM_COLOR_SPACE,
    IMG_FMT_NV12_CUSTOM_COLOR_SPACE,
    IMG_FMT_444A,
} VideoImageFormat;

/** VideoFrameParam defines the basic parameters of video frame */
typedef struct VideoFrameParam {
    VideoImageFormat format;
    uint32_t width;
    uint32_t height;
} VideoFrameParam;

/** VideoFrame defines the full parameters of video frame */
typedef struct VideoFrame : public VideoFrameParam {
    uint32_t disp_width;
    uint32_t disp_height;
    uint32_t stride[4];
    uint8_t *planes[4];
    uint32_t bits_per_sample; /** for packed formats */
    void *context;
    uint64_t timestamp;
    bool is_own_buf; /**< flag of own video plane buffers*/
    VideoFrame() {
        /** do nothing */
    }
    VideoFrame(const VideoFrame &origin) {
        *this = origin;
        is_own_buf = true;
        uint32_t luma_len = stride[0] * height * (bits_per_sample > 8 ? 2 : 1);
        planes[0] = new uint8_t[luma_len];
        if (planes[0])
            memcpy(planes[0], origin.planes[0], luma_len);
        planes[1] = new uint8_t[luma_len];
        if (planes[1]) {  // TODO: only support 420
            memcpy(planes[1], origin.planes[1], luma_len >> 2);
        }
        planes[2] = new uint8_t[luma_len];
        if (planes[2]) {  // TODO: only support 420
            memcpy(planes[2], origin.planes[2], luma_len >> 2);
        }
        if (origin.planes[3]) {
            planes[3] = new uint8_t[luma_len];
            if (planes[3]) {  // aloha channel
                memcpy(planes[3], origin.planes[3], luma_len);
            }
        }
    }
    ~VideoFrame() {
        if (is_own_buf) {
            if (planes[0]) {
                delete[] planes[0];
                planes[0] = nullptr;
            }
            if (planes[1]) {
                delete[] planes[1];
                planes[1] = nullptr;
            }
            if (planes[2]) {
                delete[] planes[2];
                planes[2] = nullptr;
            }
            if (planes[3]) {
                delete[] planes[3];
                planes[3] = nullptr;
            }
        }
    }
} VideoFrame;

#endif  //_SVT_TEST_VIDEO_FRAME_H_
