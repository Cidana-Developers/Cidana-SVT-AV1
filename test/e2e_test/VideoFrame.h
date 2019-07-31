/*
 * Copyright(c) 2019 Netflix, Inc.
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
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define INVALID_QP (0xFF)

/** VideoColorFormat defines the format of YUV video */
typedef enum VideoColorFormat {
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
} VideoColorFormat;

static inline bool is_ten_bits(VideoColorFormat fmt) {
    switch (fmt) {
    case IMG_FMT_420P10_PACKED:
    case IMG_FMT_422P10_PACKED:
    case IMG_FMT_444P10_PACKED: return true;
    default: break;
    }
    return false;
}

static const uint32_t CRC32_TABLE[256] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

/** VideoFrameParam defines the basic parameters of video frame */
typedef struct VideoFrameParam {
    VideoColorFormat format; /**< video format type */
    uint32_t width;          /**< width of video frame in pixel */
    uint32_t height;         /**< height of video frame in pixel */
    VideoFrameParam() {
        format = IMG_FMT_YV12;
        width = 0;
        height = 0;
    }
} VideoFrameParam;

/** VideoFrame defines the full parameters of video frame */
typedef struct VideoFrame : public VideoFrameParam {
    uint32_t disp_width;      /**< width to display*/
    uint32_t disp_height;     /**< height to display*/
    uint32_t stride[4];       /**< stride in array */
    uint8_t *planes[4];       /**< plane pointer to buffer address*/
    uint32_t bits_per_sample; /**< bit for each sample, default is 8, more for
                                 packed formats */
    void *context;            /**< private data context from creator */
    uint64_t timestamp;       /**< timestamp(index) of this frame */
    uint8_t *buffer;          /**< self own buffer */
    uint32_t buf_size;        /**< buffer size in bytes */
    uint32_t qp;              /**< qp of this frame */
    uint32_t checksum[4];     /**< checksum of planes in video frame buffer */
    uint32_t plane_width[4];  /**< width of planes in pixel */
    uint32_t plane_height[4]; /**< height of planes in pixel */
    VideoFrame() {
        disp_width = 0;
        disp_height = 0;
        memset(&stride, 0, sizeof(stride));
        memset(&planes, 0, sizeof(planes));
        bits_per_sample = 8;
        context = nullptr;
        timestamp = 0;
        buffer = nullptr;
        buf_size = 0;
        qp = INVALID_QP;
        memset(&checksum, 0xFF, sizeof(checksum));
        memset(&plane_width, 0, sizeof(plane_width));
        memset(&plane_height, 0, sizeof(plane_height));
    }
    VideoFrame(const VideoFrameParam &param) {
        // copy basic info from param
        *(VideoFrameParam *)this = param;
        disp_width = param.width;
        disp_height = param.height;
        memset(&stride, 0, sizeof(stride));
        memset(&planes, 0, sizeof(planes));
        bits_per_sample = is_ten_bits(param.format) ? 10 : 8;
        context = nullptr;
        timestamp = 0;
        buffer = nullptr;
        buf_size = 0;
        qp = INVALID_QP;
        memset(&checksum, 0xFF, sizeof(checksum));
        memset(&plane_width, 0, sizeof(plane_width));
        memset(&plane_height, 0, sizeof(plane_height));

        // allocate memory for new frame
        uint32_t max_size = calculate_max_frame_size(param);
        buffer = new uint8_t[max_size];
        if (buffer) {
            memset(buffer, 0, max_size);
            planes[0] = buffer;
            planes[1] = buffer + (max_size >> 2);
            planes[2] = buffer + (max_size >> 1);
            planes[3] = buffer + (max_size * 3 / 4);
            calculate_strides(param, stride);
            calculate_plane_size(param, plane_width, plane_height);
            buf_size = max_size;
        } else
            printf("video frame buffer is out of memory!!\n");
    }
    VideoFrame(const VideoFrame &origin) {
        // copy from origin
        *this = origin;
        // maintain own buffer
        const uint32_t luma_len = stride[0] * height;
        /** create video frame buffer with maximun size in 4 planes */
        uint32_t max_size = calculate_max_frame_size(origin);
        buffer = new uint8_t[max_size];
        if (buffer) {
            memset(buffer, 0, max_size);
            for (size_t i = 0; i < 4; ++i) {
                if (i != 3 || origin.planes[i]) {
                    const int buffer_len =
                        (i == 1 || i == 2) ? luma_len >> 2 : luma_len;
                    planes[i] = buffer + (i * luma_len);
                    memcpy(planes[i], origin.planes[i], buffer_len);
                }
            }
            buf_size = max_size;
        } else
            printf("video frame buffer is out of memory!!\n");
    }
    ~VideoFrame() {
        if (buf_size) {
            delete[] buffer;
            buffer = nullptr;
            memset(&planes, 0, sizeof(planes));
            buf_size = 0;
        }
    }
    /** Trim video frame buffer size for memory useage by using CRC32 for
     * every planes */
    void trim_buffer() {
        for (size_t i = 0; i < 4; i++) {
            if (planes[i]) {
                if (bits_per_sample == 8) {
                    checksum[i] = gen_crc32(
                        planes[i], plane_width[i], plane_height[i], stride[i]);
                } else {
                    checksum[i] = gen_crc32((const uint16_t *)planes[i],
                                            plane_width[i],
                                            plane_height[i],
                                            stride[i]);
                }
            }
        }
        if (buf_size) {
            delete[] buffer;
            buffer = nullptr;
            buf_size = 0;
            memset(planes, 0, sizeof(planes));
        }
    }
    static uint32_t calculate_max_frame_size(const VideoFrameParam &param) {
        uint32_t luma_size = param.width * param.height;
        return 4 * luma_size;
    }
    static uint32_t calculate_max_frame_size(const VideoFrame &frame) {
        uint32_t luma_size = frame.stride[0] * frame.height;
        return 4 * luma_size;
    }
    static void calculate_strides(const VideoFrameParam &param,
                                  uint32_t strides[4]) {
        strides[0] = param.width;
        strides[3] = 0;
        switch (param.format) {
        case IMG_FMT_420:
        case IMG_FMT_420P10_PACKED:
        case IMG_FMT_NV12:
        case IMG_FMT_YV12_CUSTOM_COLOR_SPACE:
        case IMG_FMT_NV12_CUSTOM_COLOR_SPACE:
        case IMG_FMT_422:
        case IMG_FMT_422P10_PACKED:
            strides[1] = strides[2] = param.width >> 1;
            break;
        case IMG_FMT_444A: strides[3] = param.width;
        case IMG_FMT_444:
        case IMG_FMT_444P10_PACKED:
            strides[1] = strides[2] = param.width;
            break;
        default: assert(0); break;
        }
    }
    static void calculate_plane_size(const VideoFrameParam &param,
                                     uint32_t width[4], uint32_t height[4]) {
        width[0] = param.width;
        height[0] = param.height;
        width[3] = 0;
        height[3] = 0;
        switch (param.format) {
        case IMG_FMT_420:
        case IMG_FMT_420P10_PACKED:
        case IMG_FMT_NV12:
        case IMG_FMT_YV12_CUSTOM_COLOR_SPACE:
        case IMG_FMT_NV12_CUSTOM_COLOR_SPACE:
            width[1] = width[2] = param.width >> 1;
            height[1] = height[2] = param.height >> 1;
            break;
        case IMG_FMT_422:
        case IMG_FMT_422P10_PACKED:
            width[1] = width[2] = param.width >> 1;
            height[1] = height[2] = param.height;
            break;
        case IMG_FMT_444A: width[3] = param.width; height[3] = param.height;
        case IMG_FMT_444:
        case IMG_FMT_444P10_PACKED:
            width[1] = width[2] = param.width;
            height[1] = height[2] = param.height;
            break;
        default: assert(0); break;
        }
    }
    static uint32_t gen_crc32(const uint8_t *buffer, const uint32_t width,
                              const uint32_t height, const uint32_t stride) {
        uint32_t crc32 = 0xFFFFFFFF;
        for (size_t h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                crc32 =
                    (crc32 << 8) ^
                    CRC32_TABLE[((crc32 >> 24) ^ *(buffer + (stride * h) + w)) &
                                0xFF];
            }
        }
        return crc32;
    }
    static uint32_t gen_crc32(const uint16_t *buffer, const uint32_t width,
                              const uint32_t height, const uint32_t stride) {
        uint32_t crc32 = 0xFFFFFFFF;
        for (size_t h = 0; h < height; h++) {
            for (size_t w = 0; w < width; w++) {
                crc32 = (crc32 << 8) ^
                        CRC32_TABLE[((crc32 >> 24) ^
                                     (uint8_t)(
                                         *(buffer + ((stride >> 1) * h) + w)) &
                                         0xFF) &
                                    0xFF];
            }
        }
        return crc32;
    }

} VideoFrame;

#endif  //_SVT_TEST_VIDEO_FRAME_H_
