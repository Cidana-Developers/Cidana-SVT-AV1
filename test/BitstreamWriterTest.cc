/*
 * Copyright(c) 2019 Intel Corporation
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
#include <stdlib.h>
#include <math.h>
#include <random>
#include "gtest/gtest.h"
#include "BitstreamReaderMock.h"
#include "acm_random.h"


using libaom_test::ACMRandom;
namespace {
const int num_tests = 10;
}

TEST(BitstreamWriter, TestRandomBits) {
    for (int n = 0; n < num_tests; ++n) {
        // we generate various proba
        for (int prob_gen_method = 0; prob_gen_method <= 6; ++prob_gen_method) {
            const int total_bits = 1000;
            uint8_t probas[total_bits];
            const int seed = 0xa42b;  // deterministic seeds
            std::mt19937 gen(seed);
            std::uniform_int_distribution<> prob_uni_dist(0, 255);
            std::uniform_int_distribution<> lowprob_uni_dist(0, 32);
            std::bernoulli_distribution flip_dist(0.5);

            switch (prob_gen_method) {
                // extreme probas
            case 0: memset(probas, 0, sizeof(probas)); break;
            case 1:
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = 255;
                break;
                //
            case 2:
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = 128;
                break;
            case 3:
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = prob_uni_dist(gen);
                break;
            case 4:
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = lowprob_uni_dist(gen);
                break;
            case 5:
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = 255 - lowprob_uni_dist(gen);
                break;
            default:
                for (int i = 0; i < total_bits; ++i) {
                    bool flip = flip_dist(gen);
                    probas[i] = flip ? lowprob_uni_dist(gen)
                                     : 255 - lowprob_uni_dist(gen);
                }
                break;
            }

            for (int bit_gen_method = 0; bit_gen_method < 3; ++bit_gen_method) {
                const int buffer_size = 10000;
                aom_writer bw;
                uint8_t bw_buffer[buffer_size];
                std::bernoulli_distribution bit_dist(0.5);
                uint8_t test_bits[total_bits];

                // setup test bits
                switch (bit_gen_method) {
                case 0: memset(test_bits, 0, sizeof(test_bits)); break;
                case 1:
                    for (int i = 0; i < total_bits; ++i)
                        test_bits[i] = 1;
                default:
                    for (int i = 0; i < total_bits; ++i)
                        test_bits[i] = bit_dist(gen);
                }

                // encode the bits
                aom_start_encode(&bw, bw_buffer);
                for (int i = 0; i < total_bits; ++i) {
                    aom_write(&bw, test_bits[i], static_cast<int>(probas[i]));
                }
                aom_stop_encode(&bw);

                // read out the bits and verify
                aom_reader br;
                aom_reader_init(&br, bw_buffer, bw.pos);
                for (int i = 0; i < total_bits; ++i) {
                    GTEST_ASSERT_EQ(aom_read(&br, probas[i], NULL),
                                    test_bits[i])
                        << "pos: " << i << " / " << total_bits
                        << " bit_gen_method: " << bit_gen_method
                        << " prob_gen_method: " << prob_gen_method;
                }
            }
        }
    }
}

#define FRAC_DIFF_TOTAL_ERROR 0.18
TEST(AV1, TestTell) {
  const int kBufferSize = 10000;
  aom_writer bw;
  uint8_t bw_buffer[kBufferSize];
  const int kSymbols = 1024;
  // Coders are noisier at low probabilities, so we start at p = 4.
  for (int p = 4; p < 256; p++) {
    double probability = p / 256.;
    aom_start_encode(&bw, bw_buffer);
    for (int i = 0; i < kSymbols; i++) {
      aom_write(&bw, 0, p);
    }
    aom_stop_encode(&bw);
    aom_reader br;
    aom_reader_init(&br, bw_buffer, bw.pos);
    uint32_t last_tell = aom_reader_tell(&br);
    uint32_t last_tell_frac = aom_reader_tell_frac(&br);
    double frac_diff_total = 0;
    GTEST_ASSERT_GE(aom_reader_tell(&br), 0u);
    GTEST_ASSERT_LE(aom_reader_tell(&br), 1u);
    ASSERT_FALSE(aom_reader_has_overflowed(&br));
    for (int i = 0; i < kSymbols; i++) {
      aom_read(&br, p, NULL);
      uint32_t tell = aom_reader_tell(&br);
      uint32_t tell_frac = aom_reader_tell_frac(&br);
      GTEST_ASSERT_GE(tell, last_tell)
          << "tell: " << tell << ", last_tell: " << last_tell;
      GTEST_ASSERT_GE(tell_frac, last_tell_frac)
          << "tell_frac: " << tell_frac
          << ", last_tell_frac: " << last_tell_frac;
      // Frac tell should round up to tell.
      GTEST_ASSERT_EQ(tell, (tell_frac + 7) >> 3);
      last_tell = tell;
      frac_diff_total +=
          fabs(((tell_frac - last_tell_frac) / 8.0) + log2(probability));
      last_tell_frac = tell_frac;
    }
    const uint32_t expected = (uint32_t)(-kSymbols * log2(probability));
    // Last tell should be close to the expected value.
    GTEST_ASSERT_LE(last_tell, expected + 20) << " last_tell: " << last_tell;
    // The average frac_diff error should be pretty small.
    GTEST_ASSERT_LE(frac_diff_total / kSymbols, FRAC_DIFF_TOTAL_ERROR)
        << " frac_diff_total: " << frac_diff_total;
    ASSERT_FALSE(aom_reader_has_overflowed(&br));
  }
}

TEST(AV1, TestHasOverflowed) {
  const int kBufferSize = 10000;
  aom_writer bw;
  uint8_t bw_buffer[kBufferSize];
  const int kSymbols = 1024;
  // Coders are noisier at low probabilities, so we start at p = 4.
  for (int p = 4; p < 256; p++) {
    aom_start_encode(&bw, bw_buffer);
    for (int i = 0; i < kSymbols; i++) {
      aom_write(&bw, 1, p);
    }
    aom_stop_encode(&bw);
    aom_reader br;
    aom_reader_init(&br, bw_buffer, bw.pos);
    ASSERT_FALSE(aom_reader_has_overflowed(&br));
    for (int i = 0; i < kSymbols; i++) {
      GTEST_ASSERT_EQ(aom_read(&br, p, NULL), 1);
      ASSERT_FALSE(aom_reader_has_overflowed(&br));
    }
    // In the worst case, the encoder uses just a tiny fraction of the last
    // byte in the buffer. So to guarantee that aom_reader_has_overflowed()
    // returns true, we have to consume very nearly 8 additional bits of data.
    // In the worse case, one of the bits in that byte will be 1, and the rest
    // will be zero. Once we are past that 1 bit, when the probability of
    // reading zero symbol from aom_read() is high, each additional symbol read
    // will consume very little additional data (in the case that p == 255,
    // approximately -log_2(255/256) ~= 0.0056 bits). In that case it would
    // take around 178 calls to consume more than 8 bits. That is only an upper
    // bound. In practice we are not guaranteed to hit the worse case and can
    // get away with 174 calls.
    for (int i = 0; i < 174; i++) {
      aom_read(&br, p, NULL);
    }
    ASSERT_TRUE(aom_reader_has_overflowed(&br));
  }
}
