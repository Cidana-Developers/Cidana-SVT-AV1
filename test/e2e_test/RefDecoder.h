/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
#ifndef _REF_DECODER_H_
#define _REF_DECODER_H_

#include <memory.h>
#include <stdio.h>
#include "aom/aom_decoder.h"
#include "aom/aomdx.h"
#include "VideoFrame.h"

class RefDecoder {
  public:
    typedef enum {
        /*!\brief Operation completed without error */
        REF_CODEC_OK,

        /*!\brief Unspecified error */
        REF_CODEC_ERROR = 0 - AOM_CODEC_ERROR,

        /*!\brief Memory operation failed */
        REF_CODEC_MEM_ERROR = 0 - AOM_CODEC_MEM_ERROR,

        /*!\brief ABI version mismatch */
        REF_CODEC_ABI_MISMATCH = 0 - AOM_CODEC_ABI_MISMATCH,

        /*!\brief Algorithm does not have required capability */
        REF_CODEC_INCAPABLE = 0 - AOM_CODEC_INCAPABLE,

        /*!\brief The given bitstream is not supported.
         *
         * The bitstream was unable to be parsed at the highest level. The
         * decoder is unable to proceed. This error \ref SHOULD be treated as
         * fatal to the stream. */
        REF_CODEC_UNSUP_BITSTREAM = 0 - AOM_CODEC_UNSUP_BITSTREAM,

        /*!\brief Encoded bitstream uses an unsupported feature
         *
         * The decoder does not implement a feature required by the encoder.
         * This return code should only be used for features that prevent future
         * pictures from being properly decoded. This error \ref MAY be treated
         * as fatal to the stream or \ref MAY be treated as fatal to the current
         * GOP.
         */
        REF_CODEC_UNSUP_FEATURE = 0 - AOM_CODEC_UNSUP_FEATURE,

        /*!\brief The coded data for this stream is corrupt or incomplete
         *
         * There was a problem decoding the current frame.  This return code
         * should only be used for failures that prevent future pictures from
         * being properly decoded. This error \ref MAY be treated as fatal to
         * the stream or \ref MAY be treated as fatal to the current GOP. If
         * decoding is continued for the current GOP, artifacts may be present.
         */
        REF_CODEC_CORRUPT_FRAME = 0 - AOM_CODEC_CORRUPT_FRAME,

        /*!\brief An application-supplied parameter is not valid.
         *
         */
        REF_CODEC_INVALID_PARAM = 0 - AOM_CODEC_INVALID_PARAM,

        /*!\brief An iterator reached the end of list.
         *
         */
        REF_CODEC_LIST_END = 0 - AOM_CODEC_LIST_END,

        /*!\brief Decoder need more input data to generate frame
         *
         */
        REF_CODEC_NEED_MORE_INPUT = -100,

    } RefDecoderErr;

  public:
    RefDecoder(RefDecoderErr &ret) {
        memset(&codec_, 0, sizeof(codec_));
        aom_codec_err_t err =
            aom_codec_dec_init(&codec_, aom_codec_av1_dx(), nullptr, 0);
        if (err != REF_CODEC_OK) {
            printf("can not create refernece decoder!!");
        }
        ret = (RefDecoderErr)(0 - err);
    }
    virtual ~RefDecoder() {
        aom_codec_destroy(&codec_);
    }

  public:
    RefDecoderErr process_data(const uint8_t *data, uint32_t size) {
        aom_codec_err_t err = aom_codec_decode(&codec_, data, size, nullptr);
        if (err != REF_CODEC_OK) {
            printf("decoder decode error: %d!", err);
            return (RefDecoderErr)(0 - err);
        }
        return REF_CODEC_OK;
    }
    RefDecoderErr get_frame(VideoFrame &frame) {
		aom_image_t *img = aom_codec_get_frame(&codec_, (aom_codec_iter_t *)&frame.context);
        if (img == nullptr) {
            return REF_CODEC_NEED_MORE_INPUT;
        }
		trans_video_frame(img, frame);
        return REF_CODEC_OK;
    }

  private:
    void trans_video_frame(const aom_image_t *image, VideoFrame &frame);

  protected:
    aom_codec_ctx_t codec_;
};

RefDecoder *create_reference_decoder();

#endif  // !_REF_DECODER_H_
