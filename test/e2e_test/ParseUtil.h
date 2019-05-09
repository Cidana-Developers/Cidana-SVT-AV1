/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

/******************************************************************************
 * @file ParseUtil.h
 *
 * @brief parsing utils, including
 * 1. ivf file reader
 * 2. obu reader
 * 3. sequence header parser
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/
#ifndef _PARSE_UTIL_H_
#define _PARSE_UTIL_H_

#include <math.h>
#include <stdio.h>
#include "EbDefinitions.h"
#include "aom/aom_image.h"

#define MAX_NUM_OP_POINTS 32

namespace svt_av1_e2e_tools {

typedef struct aom_timing {
    uint32_t num_units_in_display_tick;
    uint32_t time_scale;
    int equal_picture_interval;
    uint32_t num_ticks_per_picture;
} aom_timing_info_t;

typedef struct aom_dec_model_info {
    uint32_t num_units_in_decoding_tick;
    int encoder_decoder_buffer_delay_length;
    int buffer_removal_delay_length;
    int frame_presentation_delay_length;
} aom_dec_model_info_t;

typedef struct aom_dec_model_op_parameters {
    int decoder_model_present_for_this_op;
    int64_t bitrate;
    int64_t buffer_size;
    int decoder_buffer_delay;
    int encoder_buffer_delay;
    int low_delay_mode_flag;
    int initial_display_delay_present_for_this_op;
    int initial_display_delay;
} aom_dec_model_op_parameters_t;

typedef struct aom_op_timing_info_t {
    int64_t buffer_removal_delay;
} aom_op_timing_info_t;

typedef struct SequenceHeader {
    int error;  // indicate what kind of error on parsing the headers
    int ready;  // 1 - valid sequence header;
                // 0 - invalid sequence header

    int still_picture;                 // Video is a single frame still picture
    int reduced_still_picture_header;  // Use reduced header for still picture

    /* timing info */
    int timing_info_present_flag;
    int decoder_model_info_present_flag;
    aom_timing_info_t timing_info;
    aom_dec_model_info_t buffer_model;

    /* operating points */
    aom_dec_model_op_parameters_t op_params[MAX_NUM_OPERATING_POINTS + 1];
    aom_op_timing_info_t op_frame_timing[MAX_NUM_OPERATING_POINTS + 1];
    int operating_points_cnt_minus_1;
    int operating_point_idc[MAX_NUM_OPERATING_POINTS];
    int initial_display_delay_present_flag;
    uint8_t tier[MAX_NUM_OPERATING_POINTS];  // seq_tier in the spec. One bit: 0

    /* profile and levels */
    BITSTREAM_PROFILE profile;
    uint8_t seq_level_idx;
    unsigned int number_temporal_layers;
    unsigned int number_spatial_layers;
    BitstreamLevel level[MAX_NUM_OPERATING_POINTS];

    /* resolution and superblock size */
    int frame_width_bits;
    int frame_height_bits;
    int max_frame_width;
    int max_frame_height;
    block_size sb_size;  // Size of the superblock used for this frame
    int mib_size;        // Size of the superblock in units of MI blocks
    int mib_size_log2;   // Log 2 of above.

    /* frame id */
    int frame_id_numbers_present_flag;
    int frame_id_length;
    int delta_frame_id_length;

    /* coding tools */
    int order_hint_bits;
    int force_screen_content_tools;  // 0 - force off
                                     // 1 - force on
                                     // 2 - adaptive
    int force_integer_mv;          // 0 - Not to force. MV can be in 1/4 or 1/8
                                   // 1 - force to integer
                                   // 2 - adaptive
    int enable_filter_intra;       // enables/disables filterintra
    int enable_intra_edge_filter;  // enables/disables corner/edge/upsampling
    int enable_interintra_compound;  // enables/disables interintra_compound
    int enable_masked_compound;      // enables/disables masked compound
    int enable_dual_filter;          // 0 - disable dual interpolation filter
                                     // 1 - enable vert/horiz filter selection
    int enable_order_hint;     // 0 - disable order hint, and related tools
                               // jnt_comp, ref_frame_mvs, frame_sign_bias
                               // if 0, enable_jnt_comp and
                               // enable_ref_frame_mvs must be set zs 0.
    int enable_jnt_comp;       // 0 - disable joint compound modes
                               // 1 - enable it
    int enable_ref_frame_mvs;  // 0 - disable ref frame mvs
                               // 1 - enable it
    int enable_warped_motion;  // 0 - disable warped motion for sequence
                               // 1 - enable it for the sequence
    int enable_superres;  // 0 - Disable superres for the sequence, and disable
                          //     transmitting per-frame superres enabled flag.
                          // 1 - Enable superres for the sequence, and also
                          //     enable per-frame flag to denote if superres is
                          //     enabled for that frame.
    int enable_cdef;      // To turn on/off CDEF
    int enable_restoration;  // To turn on/off loop restoration
    int film_grain_params_present;

    /* color config */
    int monochrome;  // Monochorme video
    int bit_depth;
    int color_range;
    int subsampling_x;
    int subsampling_y;
    int separate_uv_delta_q;
    aom_color_primaries_t color_primaries;
    aom_transfer_characteristics_t transfer_characteristics;
    aom_matrix_coefficients_t matrix_coefficients;
    aom_chroma_sample_position_t chroma_sample_position;
} SequenceHeader;

typedef void (*aom_rb_error_handler)(void *data);

struct aom_read_bit_buffer {
    const uint8_t *bit_buffer;
    const uint8_t *bit_buffer_end;
    uint32_t bit_offset;

    void *error_handler_data;
    aom_rb_error_handler error_handler;
};

// ivf reader
#define IVF_FRAME_HDR_SZ (4 + 8) /* 4 byte size + 8 byte timestamp */
#define IVF_FILE_HDR_SZ 32
#define RAW_FRAME_HDR_SZ sizeof(uint32_t)

struct AvxRational {
    int numerator;
    int denominator;
};

struct AvxInputContext {
    const char *filename;
    FILE *file;
    uint32_t width;
    uint32_t height;
    uint32_t fourcc;
    struct AvxRational framerate;
};

typedef int64_t aom_codec_pts_t;
int file_is_ivf(struct AvxInputContext *input_ctx);
int ivf_read_frame(FILE *infile, uint8_t **buffer, size_t *bytes_read,
                   size_t *buffer_size, aom_codec_pts_t *pts);

typedef enum ATTRIBUTE_PACKED {
    OBU_SEQUENCE_HEADER = 1,
    OBU_TEMPORAL_DELIMITER = 2,
    OBU_FRAME_HEADER = 3,
    OBU_TILE_GROUP = 4,
    OBU_METADATA = 5,
    OBU_FRAME = 6,
    OBU_REDUNDANT_FRAME_HEADER = 7,
    OBU_PADDING = 15,
} obuType;

/* obu reader */
typedef struct {
    size_t size;  // Size (1 or 2 bytes) of the OBU header (including the
                  // optional OBU extension header) in the bitstream.
    obuType type;
    int has_size_field;
    int has_extension;
    // The following fields come from the OBU extension header and therefore are
    // only used if has_extension is true.
    int temporal_layer_id;
    int spatial_layer_id;
} ObuHeader;

#define OBU_HEADER_SIZE 1
#define OBU_EXTENSION_SIZE 1
#define OBU_MAX_LENGTH_FIELD_SIZE 8
int aom_uleb_decode(const uint8_t *buffer, size_t available, uint64_t *value,
                    size_t *length);
aom_codec_err_t read_obu_header(struct aom_read_bit_buffer *rb, int is_annexb,
                                ObuHeader *header);
uint32_t read_sequence_header_obu(SequenceHeader *seq_params,
                                  struct aom_read_bit_buffer *rb);
}  // namespace svt_av1_e2e_tools

#endif  // _PARSE_UTIL_H_
