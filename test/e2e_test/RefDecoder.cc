/*
 * Copyright(c) 2019 Netflix, Inc.
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
#include <stdlib.h>
#include "aom/aom_decoder.h"
#include "aom/aomdx.h"
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
extern "C" {
#define MAX_SEGMENTS 8
typedef struct insp_mv insp_mv;

struct insp_mv {
    int16_t row;
    int16_t col;
};

typedef struct insp_mi_data insp_mi_data;

struct insp_mi_data {
    insp_mv mv[2];
    int16_t ref_frame[2];
    int16_t mode;
    int16_t uv_mode;
    int16_t sb_type;
    int16_t skip;
    int16_t segment_id;
    int16_t dual_filter_type;
    int16_t filter[2];
    int16_t tx_type;
    int16_t tx_size;
    int16_t cdef_level;
    int16_t cdef_strength;
    int16_t cfl_alpha_idx;
    int16_t cfl_alpha_sign;
    int16_t current_qindex;
};

typedef struct insp_frame_data insp_frame_data;

struct insp_frame_data {
#if CONFIG_ACCOUNTING
    Accounting *accounting;
#endif
    insp_mi_data *mi_grid;
    int show_frame;
    int frame_type;
    int base_qindex;
    int mi_rows;
    int mi_cols;
    int tile_mi_rows;
    int tile_mi_cols;
    int16_t y_dequant[MAX_SEGMENTS][2];
    int16_t u_dequant[MAX_SEGMENTS][2];
    int16_t v_dequant[MAX_SEGMENTS][2];
    // TODO(negge): add per frame CDEF data
    int delta_q_present_flag;
    int delta_q_res;
};

void ifd_init(insp_frame_data *fd, int frame_width, int frame_height);
void ifd_clear(insp_frame_data *fd);
int ifd_inspect(insp_frame_data *fd, void *decoder);

}  // extern "C"

static insp_frame_data frame_data = {0};
void inspect(void *pbi, void *data) {
#if 0
    /* Fetch frame data. */
    ifd_inspect(&frame_data, pbi);
    (void)data;
    // We allocate enough space and hope we don't write out of bounds. Totally
    // unsafe but this speeds things up, especially when compiled to Javascript.
    char *buffer = aom_malloc(MAX_BUFFER);
    char *buf = buffer;
    buf += put_str(buf, "{\n");
    if (layers & BLOCK_SIZE_LAYER) {
        buf += put_block_info(buf,
                              block_size_map,
                              "blockSize",
                              offsetof(insp_mi_data, sb_type),
                              0);
    }
    if (layers & TRANSFORM_SIZE_LAYER) {
        buf += put_block_info(buf,
                              tx_size_map,
                              "transformSize",
                              offsetof(insp_mi_data, tx_size),
                              0);
    }
    if (layers & TRANSFORM_TYPE_LAYER) {
        buf += put_block_info(buf,
                              tx_type_map,
                              "transformType",
                              offsetof(insp_mi_data, tx_type),
                              0);
    }
    if (layers & DUAL_FILTER_LAYER) {
        buf += put_block_info(buf,
                              dual_filter_map,
                              "dualFilterType",
                              offsetof(insp_mi_data, dual_filter_type),
                              0);
    }
    if (layers & MODE_LAYER) {
        buf += put_block_info(
            buf, prediction_mode_map, "mode", offsetof(insp_mi_data, mode), 0);
    }
    if (layers & UV_MODE_LAYER) {
        buf += put_block_info(buf,
                              uv_prediction_mode_map,
                              "uv_mode",
                              offsetof(insp_mi_data, uv_mode),
                              0);
    }
    if (layers & SKIP_LAYER) {
        buf += put_block_info(
            buf, skip_map, "skip", offsetof(insp_mi_data, skip), 0);
    }
    if (layers & FILTER_LAYER) {
        buf += put_block_info(
            buf, NULL, "filter", offsetof(insp_mi_data, filter), 2);
    }
    if (layers & CDEF_LAYER) {
        buf += put_block_info(
            buf, NULL, "cdef_level", offsetof(insp_mi_data, cdef_level), 0);
        buf += put_block_info(buf,
                              NULL,
                              "cdef_strength",
                              offsetof(insp_mi_data, cdef_strength),
                              0);
    }
    if (layers & CFL_LAYER) {
        buf += put_block_info(buf,
                              NULL,
                              "cfl_alpha_idx",
                              offsetof(insp_mi_data, cfl_alpha_idx),
                              0);
        buf += put_block_info(buf,
                              NULL,
                              "cfl_alpha_sign",
                              offsetof(insp_mi_data, cfl_alpha_sign),
                              0);
    }
    if (layers & Q_INDEX_LAYER) {
        buf += put_block_info(
            buf, NULL, "delta_q", offsetof(insp_mi_data, current_qindex), 0);
    }
    if (layers & SEGMENT_ID_LAYER) {
        buf += put_block_info(
            buf, NULL, "seg_id", offsetof(insp_mi_data, segment_id), 0);
    }
    if (layers & MOTION_VECTORS_LAYER) {
        buf += put_motion_vectors(buf);
    }
    if (layers & REFERENCE_FRAME_LAYER) {
        buf += put_block_info(buf,
                              refs_map,
                              "referenceFrame",
                              offsetof(insp_mi_data, ref_frame),
                              2);
    }
#if CONFIG_ACCOUNTING
    if (layers & ACCOUNTING_LAYER) {
        buf += put_accounting(buf);
    }
#endif
    buf += snprintf(buf, MAX_BUFFER, "  \"frame\": %d,\n", decoded_frame_count);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"showFrame\": %d,\n", frame_data.show_frame);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"frameType\": %d,\n", frame_data.frame_type);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"baseQIndex\": %d,\n", frame_data.base_qindex);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"tileCols\": %d,\n", frame_data.tile_mi_cols);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"tileRows\": %d,\n", frame_data.tile_mi_rows);
    buf += snprintf(buf,
                    MAX_BUFFER,
                    "  \"deltaQPresentFlag\": %d,\n",
                    frame_data.delta_q_present_flag);
    buf += snprintf(
        buf, MAX_BUFFER, "  \"deltaQRes\": %d,\n", frame_data.delta_q_res);
    buf += put_str(buf, "  \"config\": {");
    buf += put_map(buf, config_map);
    buf += put_str(buf, "},\n");
    buf += put_str(buf, "  \"configString\": \"");
    buf += put_str_with_escape(buf, aom_codec_build_config());
    buf += put_str(buf, "\"\n");
    decoded_frame_count++;
    buf += put_str(buf, "},\n");
    *(buf++) = 0;
    on_frame_decoded_dump(buffer);
    aom_free(buffer);
#endif
}

static VideoColorFormat trans_video_format(aom_img_fmt_t fmt) {
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
    codec_handle_ = (void *)malloc(sizeof(aom_codec_ctx_t));
    aom_codec_ctx_t *codec_ = (aom_codec_ctx_t *)codec_handle_;

    memset(codec_, 0, sizeof(aom_codec_ctx_t));
    aom_codec_err_t err =
        aom_codec_dec_init(codec_, aom_codec_av1_dx(), nullptr, 0);
    if (err != AOM_CODEC_OK) {
        printf("can not create refernece decoder!!");
    }
    ret = (RefDecoderErr)(0 - err);
    dec_frame_cnt_ = 0;
    init_timestamp_ = 0;
    frame_interval_ = 1;
}

RefDecoder::~RefDecoder() {
    aom_codec_ctx_t *codec_ = (aom_codec_ctx_t *)codec_handle_;
    aom_codec_destroy(codec_);
    free(codec_handle_);
}

RefDecoder::RefDecoderErr RefDecoder::setup(const uint64_t init_ts,
                                            const uint32_t interval) {
    init_timestamp_ = init_ts;
    frame_interval_ = interval;

    ifd_init(&frame_data, 320, 140);

    aom_inspect_init ii;
    ii.inspect_cb = inspect;
    ii.inspect_ctx = this;
    aom_codec_control(
        (aom_codec_ctx_t *)codec_handle_, AV1_SET_INSPECTION_CALLBACK, &ii);
    return REF_CODEC_OK;
}

RefDecoder::RefDecoderErr RefDecoder::process_data(const uint8_t *data,
                                                   const uint32_t size) {
    aom_codec_ctx_t *codec_ = (aom_codec_ctx_t *)codec_handle_;

    aom_codec_err_t err = aom_codec_decode(codec_, data, size, nullptr);
    if (err != AOM_CODEC_OK) {
        printf("decoder decode error: %d!", err);
        return (RefDecoderErr)(0 - err);
    }
    return REF_CODEC_OK;
}

RefDecoder::RefDecoderErr RefDecoder::get_frame(VideoFrame &frame) {
    aom_codec_ctx_t *codec_ = (aom_codec_ctx_t *)codec_handle_;

    aom_image_t *img =
        aom_codec_get_frame(codec_, (aom_codec_iter_t *)&frame.context);
    if (img == nullptr) {
        return REF_CODEC_NEED_MORE_INPUT;
    }
    trans_video_frame(img, frame);
    dec_frame_cnt_++;
    return REF_CODEC_OK;
}

void RefDecoder::trans_video_frame(const void *image_handle,
                                   VideoFrame &frame) {
    if (image_handle == nullptr)
        return;

    const aom_image_t *image = (const aom_image_t *)image_handle;
    frame.format = trans_video_format(image->fmt);
    frame.width = image->w;
    frame.height = image->h;
    frame.disp_width = image->d_w;
    frame.disp_height = image->d_h;
    memcpy(frame.stride, image->stride, sizeof(frame.stride));
    memcpy(frame.planes, image->planes, sizeof(frame.planes));
    frame.bits_per_sample = image->bit_depth;
    frame.timestamp =
        init_timestamp_ + ((uint64_t)dec_frame_cnt_ * frame_interval_);
}
