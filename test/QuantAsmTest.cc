/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <random>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gtest/gtest.h"

// Workaround to eliminate the compiling warning on linux
// The macro will conflict with definition in gtest.h
#ifdef __USE_GNU
#undef __USE_GNU  // defined in EbThreads.h
#endif
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE  // defined in EbThreads.h
#endif

#include "EbDefinitions.h"
#include "EbTransforms.h"
#include "EbPictureControlSet.h"
#include "aom_dsp_rtcd.h"
#include "util.h"

namespace {
const int deterministic_seed = 0xa42b;
extern "C" void av1_build_quantizer(aom_bit_depth_t bit_depth,
                                    int32_t y_dc_delta_q, int32_t u_dc_delta_q,
                                    int32_t u_ac_delta_q, int32_t v_dc_delta_q,
                                    int32_t v_ac_delta_q, Quants *const quants,
                                    Dequants *const deq);
extern "C" void *aom_memalign(size_t align, size_t size);
extern "C" void aom_free(void *memblk);

using QuantizeFunc = void (*)(const tran_low_t *coeff_ptr, intptr_t n_coeffs,
                              int32_t skip_block, const int16_t *zbin_ptr,
                              const int16_t *round_ptr,
                              const int16_t *quant_ptr,
                              const int16_t *quant_shift_ptr,
                              tran_low_t *qcoeff_ptr, tran_low_t *dqcoeff_ptr,
                              const int16_t *dequant_ptr, uint16_t *eob_ptr,
                              const int16_t *scan, const int16_t *iscan);

using QuantizeParam = std::tuple<int, int>;

typedef struct {
    Quants quant;
    Dequants dequant;
} QuanTable;

const int kTestNum = 10;

class QuantizeTest : public ::testing::TestWithParam<QuantizeParam> {
  protected:
    QuantizeTest()
        : gen_(deterministic_seed),
          tx_size_(static_cast<TxSize>(TEST_GET_PARAM(0))),
          bd_(static_cast<aom_bit_depth_t>(TEST_GET_PARAM(1))) {

        coeff_min_ = -(1 << (7 + bd_));
        coeff_max_ = (1 << (7 + bd_)) - 1;
        decltype(dist_nbit_)::param_type param{coeff_min_, coeff_max_};
        dist_nbit_.param(param);

        qtab_ = reinterpret_cast<QuanTable *>(aom_memalign(32, sizeof(*qtab_)));
        const int n_coeffs = coeff_num();
        coeff_ = reinterpret_cast<tran_low_t *>(
            aom_memalign(32, 6 * n_coeffs * sizeof(tran_low_t)));
        av1_build_quantizer(bd_, 0, 0, 0, 0, 0, &qtab_->quant, &qtab_->dequant);

        setup_func_ptrs();
    }

    virtual ~QuantizeTest() {
        aom_free(qtab_);
        qtab_ = NULL;
        aom_free(coeff_);
        coeff_ = NULL;
        aom_clear_system_state();
    }

    // ref setup_rtcd_internal() in aom_dsp_rtcd.h
    void setup_func_ptrs() {
        if (bd_ == AOM_BITS_8) {
            if (tx_size_ == TX_32X32) {
                quant_ref_ = aom_quantize_b_32x32_c_II;
                quant_test_ = aom_highbd_quantize_b_32x32_avx2;
            } else if (tx_size_ == TX_64X64) {
                quant_ref_ = aom_quantize_b_64x64_c_II;
                quant_test_ = aom_highbd_quantize_b_64x64_avx2;
            } else {
                quant_ref_ = aom_quantize_b_c_II;
                quant_test_ = aom_highbd_quantize_b_avx2;
            }
        } else {
            if (tx_size_ == TX_32X32) {
                quant_ref_ = aom_highbd_quantize_b_32x32_c;
                quant_test_ = aom_highbd_quantize_b_32x32_avx2;
            } else if (tx_size_ == TX_64X64) {
                quant_ref_ = aom_highbd_quantize_b_64x64_c;
                quant_test_ = aom_highbd_quantize_b_64x64_avx2;
            } else {
                quant_ref_ = aom_highbd_quantize_b_c;
                quant_test_ = aom_highbd_quantize_b_avx2;
            }
        }
    }

    void run_quantize(bool is_random_input, int q = 0, int test_num = 1) {
        tran_low_t *coeff_ptr = coeff_;
        const intptr_t n_coeffs = coeff_num();
        const int32_t skip_block = 0;

        tran_low_t *qcoeff_ref = coeff_ptr + n_coeffs;
        tran_low_t *dqcoeff_ref = qcoeff_ref + n_coeffs;
        tran_low_t *qcoeff_test = dqcoeff_ref + n_coeffs;
        tran_low_t *dqcoeff_test = qcoeff_test + n_coeffs;
        uint16_t *eob = (uint16_t *)(dqcoeff_test + n_coeffs);

        const SCAN_ORDER *const sc = &av1_scan_orders[tx_size_][DCT_DCT];
        const int16_t *zbin = qtab_->quant.y_zbin[q];
        const int16_t *round = 0;
        const int16_t *quant = 0;
        round = qtab_->quant.y_round[q];
        quant = qtab_->quant.y_quant[q];

        const int16_t *quant_shift = qtab_->quant.y_quant_shift[q];
        const int16_t *dequant = qtab_->dequant.y_dequant_QTX[q];

        for (int i = 0; i < test_num; ++i) {
            if (is_random_input) {
                fill_coeff_const(0, n_coeffs, 0);
                fill_coeff_random(0, dist_nbit_(gen_) % n_coeffs);
            }

            memset(qcoeff_ref, 0, 5 * n_coeffs * sizeof(*qcoeff_ref));

            quant_ref_(coeff_ptr,
                       n_coeffs,
                       skip_block,
                       zbin,
                       round,
                       quant,
                       quant_shift,
                       qcoeff_ref,
                       dqcoeff_ref,
                       dequant,
                       &eob[0],
                       sc->scan,
                       sc->iscan);

            quant_test_(coeff_ptr,
                        n_coeffs,
                        skip_block,
                        zbin,
                        round,
                        quant,
                        quant_shift,
                        qcoeff_test,
                        dqcoeff_test,
                        dequant,
                        &eob[1],
                        sc->scan,
                        sc->iscan);

            for (int j = 0; j < n_coeffs; ++j) {
                ASSERT_EQ(qcoeff_ref[j], qcoeff_test[j])
                    << "Q mismatch on test: " << i << " at position: " << j
                    << " Q: " << q << " coeff: " << coeff_ptr[j];
            }

            for (int j = 0; j < n_coeffs; ++j) {
                ASSERT_EQ(dqcoeff_ref[j], dqcoeff_test[j])
                    << "Dq mismatch on test: " << i << " at position: " << j
                    << " Q: " << q << " coeff: " << coeff_ptr[j];
            }

            ASSERT_EQ(eob[0], eob[1])
                << "eobs mismatch on test: " << i << " Q: " << q;
        }
    }

    int coeff_num() const {
        return av1_get_max_eob(tx_size_);
    }

    void fill_coeff_const(int i_begin, int i_end, tran_low_t c) {
        for (int i = i_begin; i < i_end; ++i) {
            coeff_[i] = c;
        }
    }

    void fill_coeff_random(int i_begin, int i_end) {
        for (int i = i_begin; i < i_end; ++i) {
            coeff_[i] = dist_nbit_(gen_);
        }
    }

    std::mt19937 gen_;
    std::uniform_int_distribution<> dist_nbit_;
    QuanTable *qtab_;
    tran_low_t *coeff_;
    tran_low_t coeff_min_;
    tran_low_t coeff_max_;
    QuantizeFunc quant_ref_;
    QuantizeFunc quant_test_;
    const TxSize tx_size_;
    const aom_bit_depth_t bd_;
};

TEST_P(QuantizeTest, input_zero_all) {
    fill_coeff_const(0, coeff_num(), 0);
    run_quantize(false, 0, 1);
}

TEST_P(QuantizeTest, input_dcac_minmax) {
    fill_coeff_const(2, coeff_num(), 0);
    fill_coeff_const(0, 1, coeff_min_);
    fill_coeff_const(1, 2, coeff_min_);
    run_quantize(false, 0, 1);
    fill_coeff_const(0, 1, coeff_min_);
    fill_coeff_const(1, 2, coeff_max_);
    run_quantize(false, 0, 1);
    fill_coeff_const(0, 1, coeff_max_);
    fill_coeff_const(1, 2, coeff_min_);
    run_quantize(false, 0, 1);
    fill_coeff_const(0, 1, coeff_max_);
    fill_coeff_const(1, 2, coeff_max_);
    run_quantize(false, 0, 1);
}

TEST_P(QuantizeTest, input_random_dc_only) {
    fill_coeff_const(1, coeff_num(), 0);
    fill_coeff_random(0, 1);
    run_quantize(false, 0, 1);
}

TEST_P(QuantizeTest, input_random_all_q_0) {
    run_quantize(true, 0, kTestNum);
}

TEST_P(QuantizeTest, input_random_all_q_all) {
    for (int q = 0; q < QINDEX_RANGE; ++q) {
        run_quantize(true, q, kTestNum);
    }
}

#if 1
INSTANTIATE_TEST_CASE_P(
    AVX2, QuantizeTest,
    ::testing::Combine(::testing::Values(static_cast<int>(TX_16X16),
                                         static_cast<int>(TX_32X32),
                                         static_cast<int>(TX_64X64)),
                       ::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10))));
#else
INSTANTIATE_TEST_CASE_P(
    AVX2, QuantizeTest,
    ::testing::Combine(::testing::Range(static_cast<int>(TX_4X4),
                                        static_cast<int>(TX_SIZES_ALL), 1),
                       ::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10))));
#endif

}  // namespace
