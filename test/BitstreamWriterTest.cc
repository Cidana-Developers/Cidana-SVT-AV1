#include <math.h>
#include <stdlib.h>
#include <random>
#include "BitstreamReaderMock.h"
#include "EbCabacContextModel.h"
#include "gtest/gtest.h"

TEST(BitstreamWriter, write_bits_random) {
    const int num_tests = 10;
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
                // uniform distribution between 0 ~ 255
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = prob_uni_dist(gen);
                break;
            case 4:
                // low probability
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = lowprob_uni_dist(gen);
                break;
            case 5:
                // high probability
                for (int i = 0; i < total_bits; ++i)
                    probas[i] = 255 - lowprob_uni_dist(gen);
                break;
            default:
                // mix high and low probability
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
                    GTEST_ASSERT_EQ(aom_read(&br, probas[i], nullptr),
                                    test_bits[i])
                        << "pos: " << i << " / " << total_bits
                        << " bit_gen_method: " << bit_gen_method
                        << " prob_gen_method: " << prob_gen_method;
                }
            }
        }
    }
}

TEST(BitstreamWriter, write_literal_extreme_int) {
    // test max int
    constexpr int max_int = std::numeric_limits<int>::max();
    constexpr int min_int = std::numeric_limits<int>::min();

    const int buffer_size = 1024;
    uint8_t stream_buffer[buffer_size];
    aom_writer bw;

    aom_start_encode(&bw, stream_buffer);
    aom_write_literal(&bw, max_int, 32);
    aom_write_literal(&bw, min_int, 32);
    aom_stop_encode(&bw);

    aom_reader br;
    aom_reader_init(&br, stream_buffer, bw.pos);
    EXPECT_EQ(aom_read_literal(&br, 32, nullptr), max_int)
        << "read max_int fail";
    EXPECT_EQ(aom_read_literal(&br, 32, nullptr), min_int)
        << "read min_int fail";
}

TEST(BitstreamWriter, write_symbol_no_update) {
    aom_writer bw = {0};
    const int buffer_size = 1024;
    uint8_t stream_buffer[buffer_size];

    // get default cdf
    const int base_qindex = 20;
    FRAME_CONTEXT fc = {0};
    av1_default_coef_probs(&fc, base_qindex);

    // write 0, 1 in order
    aom_start_encode(&bw, stream_buffer);
    aom_write_symbol(&bw, 0, fc.txb_skip_cdf[0][0], 2);
    aom_write_symbol(&bw, 1, fc.txb_skip_cdf[0][0], 2);
    aom_stop_encode(&bw);

    // expect read out 0, 1 in order
    aom_reader br;
    aom_reader_init(&br, stream_buffer, bw.pos);
    EXPECT_EQ(aom_read_symbol(&br, fc.txb_skip_cdf[0][0], 2, nullptr), 0);
    EXPECT_EQ(aom_read_symbol(&br, fc.txb_skip_cdf[0][0], 2, nullptr), 1);
}

TEST(BitstreamWriter, write_symbol_with_update) {
    aom_writer bw = {0};
    const int buffer_size = 1024;
    uint8_t stream_buffer[buffer_size];
    bw.allow_update_cdf = 1;

    // get default cdf
    const int base_qindex = 20;
    FRAME_CONTEXT fc = {0};
    av1_default_coef_probs(&fc, base_qindex);

    // write 0, 1 in order
    // TODO(wenyao): encode much more coefficients
    aom_start_encode(&bw, stream_buffer);
    aom_write_symbol(&bw, 0, fc.txb_skip_cdf[0][0], 2);
    aom_write_symbol(&bw, 1, fc.txb_skip_cdf[0][0], 2);
    aom_stop_encode(&bw);

    // expect read out 0, 1 in order
    aom_reader br;
    aom_reader_init(&br, stream_buffer, bw.pos);
    br.allow_update_cdf = 1;
    av1_default_coef_probs(&fc, base_qindex);  // reset cdf
    EXPECT_EQ(aom_read_symbol(&br, fc.txb_skip_cdf[0][0], 2, nullptr), 0);
    EXPECT_EQ(aom_read_symbol(&br, fc.txb_skip_cdf[0][0], 2, nullptr), 1);
}
