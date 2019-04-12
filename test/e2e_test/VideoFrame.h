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
} VideoFrame;

#endif  //_SVT_TEST_VIDEO_FRAME_H_