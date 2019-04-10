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
    RefDecoder(RefDecoderErr &ret);
    virtual ~RefDecoder();

  public:
    RefDecoderErr process_data(const uint8_t *data, uint32_t size);
    RefDecoderErr get_frame(VideoFrame &frame);

  private:
    void trans_video_frame(const aom_image_t *image, VideoFrame &frame);

  protected:
    aom_codec_ctx_t codec_;
	uint32_t ref_frame_cnt_;
};

RefDecoder *create_reference_decoder();

#endif  // !_REF_DECODER_H_
