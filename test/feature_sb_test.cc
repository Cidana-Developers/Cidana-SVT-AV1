/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */
#include <stdio.h>
#include "util.h"

TEST(FeatureTest, sb_size_test) {
    FILE *f = fopen("/Users/mac/Downloads/waterfall_cpu3.ivf", "rb");
    struct AvxInputContext input_ctx = {
        "/Users/mac/Downloads/waterfall_cpu3.ivf", f, 0, 0, 0, {0, 0}};
    if (file_is_ivf(&input_ctx)) {
        printf("File is ivf\n");
    }

    uint8_t *stream_buf = (uint8_t *)malloc(1024);
    size_t buf_sz = 1024;
    aom_codec_pts_t pts;
    size_t frame_sz = 0;
    int frame_cnt = 0;
    while (ivf_read_frame(f, &stream_buf, &frame_sz, &buf_sz, &pts) == 0) {
        // read one frame; process obu
        printf("frame count: %d\n", frame_cnt++);
        uint8_t *frame_buf = stream_buf;
        aom_codec_err_t err;
        do {
            struct aom_read_bit_buffer rb = {
                frame_buf, frame_buf + frame_sz, 0, NULL, NULL};
            ObuHeader ou;
            err = read_obu_header(&rb, 0, &ou);
            if (ou.has_size_field) {
                uint64_t u64_payload_length = 0;
                int header_size = ou.has_extension ? 2 : 1;
                int value_len = 0;

                frame_buf += header_size;
                frame_sz -= header_size;
                for (int len = 0; len < OBU_MAX_LENGTH_FIELD_SIZE; ++len) {
                    if ((frame_buf[len] >> 7) == 0) {
                        ++len;
                        value_len = len;
                        break;
                    }
                }
                aom_uleb_decode(
                    frame_buf, value_len, &u64_payload_length, NULL);

                frame_buf += value_len;
                frame_sz -= value_len;
                printf("OBU type: %d, payload length: %lld\n",
                       ou.type,
                       u64_payload_length);
                if (ou.type == OBU_SEQUENCE_HEADER) {
                    // check the ou type and parse sequence header
                    struct aom_read_bit_buffer rb = {
                        frame_buf, frame_buf + frame_sz, 0, NULL, NULL};
                    SequenceHeader sqs_headers = {0};
                    if (read_sequence_header_obu(&sqs_headers, &rb) == 0) {
                        printf("read seqence header fail\n");
                    }
                }

                frame_buf += u64_payload_length;
                frame_sz -= u64_payload_length;
            }
        } while (err == 0 && frame_sz > 0);
    }

    free(stream_buf);
    fclose(f);
    SUCCEED();
}
