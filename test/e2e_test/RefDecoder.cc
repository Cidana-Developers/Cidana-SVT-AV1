/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
/******************************************************************************
 * @file RefDecoder.cc
 *
 * @brief Impelmentation of reference decoder
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/

#include "RefDecoder.h"

RefDecoder *create_reference_decoder() {
    RefDecoder::RefDecoderErr ret = RefDecoder::REF_CODEC_OK;
    RefDecoder *decoder = new RefDecoder(ret);
    if (decoder && ret != RefDecoder::REF_CODEC_OK) {
        // decoder object is create but init failed
        delete decoder;
        decoder = nullptr;
    }
    return decoder;
}

static VideoImageFormat trans_video_format(aom_img_fmt_t fmt) {
    switch (fmt) {
    case AOM_IMG_FMT_YV12: return IMG_FMT_YV12;
    case AOM_IMG_FMT_I420: return IMG_FMT_NV12;
    case AOM_IMG_FMT_AOMYV12: return IMG_FMT_YV12_CUSTOM_COLOR_SPACE;
    case AOM_IMG_FMT_AOMI420: return IMG_FMT_NV12_CUSTOM_COLOR_SPACE;
    case AOM_IMG_FMT_I422: return IMG_FMT_422;
    case AOM_IMG_FMT_I444: return IMG_FMT_444;
    case AOM_IMG_FMT_444A: return IMG_FMT_444A;
    case AOM_IMG_FMT_I42016: return IMG_FMT_420P10_PACKED;
    case AOM_IMG_FMT_I42216: return IMG_FMT_422P10_PACKED;
    case AOM_IMG_FMT_I44416: return IMG_FMT_444P10_PACKED;
    default: break;
    }
    return IMG_FMT_422;
}

RefDecoder::RefDecoder(RefDecoder::RefDecoderErr &ret) {
    memset(&codec_, 0, sizeof(codec_));
    aom_codec_err_t err =
        aom_codec_dec_init(&codec_, aom_codec_av1_dx(), nullptr, 0);
    if (err != AOM_CODEC_OK) {
        printf("can not create refernece decoder!!");
    }
    ret = (RefDecoderErr)(0 - err);
    ref_frame_cnt_ = 0;
}

RefDecoder::~RefDecoder() {
    aom_codec_destroy(&codec_);
}

RefDecoder::RefDecoderErr RefDecoder::process_data(const uint8_t *data,
                                                   const uint32_t size) {
    aom_codec_err_t err = aom_codec_decode(&codec_, data, size, nullptr);
    if (err != AOM_CODEC_OK) {
        printf("decoder decode error: %d!", err);
        return (RefDecoderErr)(0 - err);
    }
    return REF_CODEC_OK;
}

RefDecoder::RefDecoderErr RefDecoder::get_frame(VideoFrame &frame) {
    aom_image_t *img =
        aom_codec_get_frame(&codec_, (aom_codec_iter_t *)&frame.context);
    if (img == nullptr) {
        return REF_CODEC_NEED_MORE_INPUT;
    }
    trans_video_frame(img, frame);
    printf("ref_frame_count %d\n", ref_frame_cnt_++);
    return REF_CODEC_OK;
}

void RefDecoder::trans_video_frame(const aom_image_t *image,
                                   VideoFrame &frame) {
    if (image == nullptr)
        return;

    frame.format = trans_video_format(image->fmt);
    frame.width = image->w;
    frame.height = image->h;
    frame.disp_width = image->d_w;
    frame.disp_height = image->d_h;
    memcpy(frame.stride, image->stride, sizeof(frame.stride));
    memcpy(frame.planes, image->planes, sizeof(frame.planes));
    frame.bits_per_sample = image->bit_depth;
    frame.timestamp = ref_frame_cnt_;
}