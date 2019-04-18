/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file CompareTools.h
 *
 * @brief Defines a tool set for comparism
 *
 * @author Cidana-Ryan Cidana-Edmond
 *
 ******************************************************************************/

#ifndef _COMPARE_TOOLS_H_
#define _COMPARE_TOOLS_H_

#include <stdint.h>
#include <math.h>
#include <float.h>
#include "ReconSink.h"

namespace svt_av1_e2e_tools {
static inline bool compare_image(const ReconSink::ReconMug *recon,
                                 const VideoFrame *ref_frame,
                                 VideoImageFormat fmt) {
    const uint32_t width = ref_frame->disp_width;
    const uint32_t height = ref_frame->disp_height;
    unsigned int i = 0;
    // Support 420 only, need add more code for 422 & 444.
    if (fmt != IMG_FMT_420 && fmt != IMG_FMT_420P10_PACKED)
        return true;
    if (ref_frame->bits_per_sample == 8) {
        for (uint32_t l = 0; l < height; l++) {
            const uint8_t *s = recon->mug_buf + l * width;
            const uint8_t *d = ref_frame->planes[0] + l * ref_frame->stride[0];
            for (uint32_t r = 0; r < width; r++) {
                if (s[r] != d[r * 2])  // ref decoder use 2bytes to store 8 bits
                                       // depth pix.
                    return false;
                i++;
            }
        }

        for (uint32_t l = 0; l < (height >> 1); l++) {
            const uint8_t *s =
                (recon->mug_buf + width * height) + l * (width >> 1);
            const uint8_t *d = ref_frame->planes[1] + l * ref_frame->stride[1];
            for (uint32_t r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r * 2])
                    return false;
                i++;
            }
        }

        for (uint32_t l = 0; l < (height >> 1); l++) {
            const uint8_t *s =
                (recon->mug_buf + width * height * 5 / 4) + l * (width >> 1);
            const uint8_t *d = ref_frame->planes[2] + l * ref_frame->stride[2];
            for (uint32_t r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r * 2])
                    return false;
                i++;
            }
        }
    } else  // 10bit mode.
    {
        for (uint32_t l = 0; l < height; l++) {
            const uint16_t *s = (uint16_t *)(recon->mug_buf + l * width * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[0] + l * ref_frame->stride[0]);
            for (uint32_t r = 0; r < width; r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }

        for (uint32_t l = 0; l < (height >> 1); l++) {
            const uint16_t *s =
                (uint16_t *)(recon->mug_buf + width * height * 2 +
                             l * (width >> 1) * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[1] + l * ref_frame->stride[1]);
            for (uint32_t r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }

        for (uint32_t l = 0; l < (height >> 1); l++) {
            const uint16_t *s =
                (uint16_t *)(recon->mug_buf + width * height * 5 / 4 * 2 +
                             l * (width >> 1) * 2);
            const uint16_t *d =
                (uint16_t *)(ref_frame->planes[2] + l * ref_frame->stride[2]);
            for (uint32_t r = 0; r < (width >> 1); r++) {
                if (s[r] != d[r])
                    return false;
                i++;
            }
        }
    }
    return true;
}

static inline double psnr_8bit(const uint8_t *p1, const uint8_t *p2,
                               const uint32_t size) {
    // Assert that, p1 p2 hase same size and no stirde issue.
    double mse = 0.0;
    for (uint32_t i = 0; i < size; i++) {
        const uint8_t I = p1[i];
        const uint8_t K = p2[i];
        const int32_t diff = I - K;
        mse += diff * diff;
    }
    mse /= size;

    double psnr = INFINITY;

    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((255 * 255) / mse);
    }
    return psnr;
}

static inline double psnr_8bit(const uint8_t *p1, const uint32_t stride1,
                               const uint8_t *p2, const uint32_t stride2,
                               const uint32_t width, const uint32_t height) {
    double mse = 0.0;
    for (size_t y = 0; y < height; y++) {
        const uint8_t *s = p1 + (y * stride1);
        /** walk-around for decoder output is in 16bit */
        const uint16_t *d = (uint16_t *)p2 + (y * stride2 / 2);
        for (size_t x = 0; x < width; x++) {
            const int32_t diff = s[x] - (d[x] & 0xFF);
            mse += diff * diff;
        }
    }
    mse /= width * height;

    double psnr = INFINITY;
    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((255 * 255) / mse);
    }
    return psnr;
}

static inline double psnr_10bit(const uint16_t *p1, const uint16_t *p2,
                                const uint32_t size) {
    // Assert that, p1 p2 hase same size and no stirde issue.
    double mse = 0.0;
    for (uint32_t i = 0; i < size; i++) {
        const uint16_t I = p1[i] & 0x3FF;
        const uint16_t K = p2[i] & 0x3FF;
        const int32_t diff = I - K;
        mse += diff * diff;
    }
    mse /= size;

    double psnr = INFINITY;

    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((1023 * 1023) / mse);
    }
    return psnr;
}

static inline double psnr_10bit(const uint16_t *p1, const uint32_t stride1,
                                const uint16_t *p2, const uint32_t stride2,
                                const uint32_t width, const uint32_t height) {
    double mse = 0.0;
    for (size_t y = 0; y < height; y++) {
        const uint16_t *s = p1 + (y * stride1);
        const uint16_t *d = p2 + (y * stride2);
        for (size_t x = 0; x < width; x++) {
            const int32_t diff = (s[x] & 0x3FF) - (d[x] & 0x3FF);
            mse += diff * diff;
        }
    }
    mse /= width * height * 2;

    double psnr = INFINITY;
    if (DBL_EPSILON < mse) {
        psnr = 10 * log10((1023 * 1023) / mse);
    }
    return psnr;
}

}  // namespace svt_av1_e2e_tools
#endif  // !_COMPARE_TOOLS_H_