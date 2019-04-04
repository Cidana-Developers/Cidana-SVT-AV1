/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
#include "RefDecoder.h"

RefDecoder *create_reference_decoder() {
    RefDecoder::RefDecoderErr ret;
    RefDecoder *decoder = new RefDecoder(ret);
    if (decoder && ret != RefDecoder::REF_CODEC_OK) {
        // decoder object is create but init failed
        delete decoder;
        decoder = nullptr;
    }
    return decoder;
}

static VideoImageFormat trans_video_format(aom_img_fmt_t fmt) {
	switch (fmt)
	{
	case AOM_IMG_FMT_YV12:
		return IMG_FMT_YV12; 
	case AOM_IMG_FMT_I420:
		return IMG_FMT_NV12;
	case AOM_IMG_FMT_AOMYV12:
		return IMG_FMT_YV12_CUSTOM_COLOR_SPACE;
	case AOM_IMG_FMT_AOMI420:
		return IMG_FMT_NV12_CUSTOM_COLOR_SPACE;
	case AOM_IMG_FMT_I422:
		return IMG_FMT_422;
	case AOM_IMG_FMT_I444:
		return IMG_FMT_444;
	case AOM_IMG_FMT_444A:
		return IMG_FMT_444A;
	case AOM_IMG_FMT_I42016:
		return IMG_FMT_420P10_PACKED;
	case AOM_IMG_FMT_I42216:
		return IMG_FMT_422P10_PACKED;
	case AOM_IMG_FMT_I44416:
		return IMG_FMT_444P10_PACKED;
	default:
		break;
	}
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
	frame.bits_per_sample = image->bps;
}
