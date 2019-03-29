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
        decltype(dist_nbit_)::param_type param{-(1 << (7 + bd_)),
                                               (1 << (7 + bd_)) - 1};
        dist_nbit_.param(param);

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
                quant_ref_ = aom_highbd_quantize_b_c;
                quant_test_ = aom_highbd_quantize_b_32x32_avx2;
            } else if (tx_size_ == TX_64X64) {
                quant_ref_ = aom_highbd_quantize_b_32x32_c;
                quant_test_ = aom_highbd_quantize_b_64x64_avx2;
            } else {
                quant_ref_ = aom_highbd_quantize_b_64x64_c;
                quant_test_ = aom_highbd_quantize_b_avx2;
            }
        }
    }

    virtual ~QuantizeTest() {
    }

    virtual void SetUp() {
        qtab_ = reinterpret_cast<QuanTable *>(aom_memalign(32, sizeof(*qtab_)));
        const int n_coeffs = coeff_num();
        coeff_ = reinterpret_cast<tran_low_t *>(
            aom_memalign(32, 6 * n_coeffs * sizeof(tran_low_t)));
        InitQuantizer();
    }

    virtual void TearDown() {
        aom_free(qtab_);
        qtab_ = NULL;
        aom_free(coeff_);
        coeff_ = NULL;
    }

    void InitQuantizer() {
        av1_build_quantizer(bd_, 0, 0, 0, 0, 0, &qtab_->quant, &qtab_->dequant);
    }

    void QuantizeRun(bool is_loop, int q = 0, int test_num = 1) {
        tran_low_t *coeff_ptr = coeff_;
        const intptr_t n_coeffs = coeff_num();
        const int32_t skip_block = 0;

        tran_low_t *qcoeff_ref = coeff_ptr + n_coeffs;
        tran_low_t *dqcoeff_ref = qcoeff_ref + n_coeffs;

        tran_low_t *qcoeff_test = dqcoeff_ref + n_coeffs;
        tran_low_t *dqcoeff_test = qcoeff_test + n_coeffs;
        uint16_t *eob = (uint16_t *)(dqcoeff_test + n_coeffs);

        // Testing uses 2-D DCT scan order table
        const SCAN_ORDER *const sc = &av1_scan_orders[tx_size_][DCT_DCT];

        // Testing uses luminance quantization table
        const int16_t *zbin = qtab_->quant.y_zbin[q];

        const int16_t *round = 0;
        const int16_t *quant = 0;
        round = qtab_->quant.y_round[q];
        quant = qtab_->quant.y_quant[q];

        const int16_t *quant_shift = qtab_->quant.y_quant_shift[q];
        const int16_t *dequant = qtab_->dequant.y_dequant_QTX[q];

        for (int i = 0; i < test_num; ++i) {
            if (is_loop)
                FillCoeffRandom();

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

    void CompareResults(const tran_low_t *buf_ref, const tran_low_t *buf_test,
                        int size, const char *text, int q, int number) {
        int i;
        for (i = 0; i < size; ++i) {
            ASSERT_EQ(buf_ref[i], buf_test[i])
                << text << " mismatch on test: " << number
                << " at position: " << i << " Q: " << q;
        }
    }

    int coeff_num() const {
        return av1_get_max_eob(tx_size_);
    }

    void FillCoeff(tran_low_t c) {
        const int n_coeffs = coeff_num();
        for (int i = 0; i < n_coeffs; ++i) {
            coeff_[i] = c;
        }
    }

    void FillCoeffRandom() {
        const int n_coeffs = coeff_num();
        FillCoeffZero();
        int num = dist_nbit_(gen_) % n_coeffs;
        for (int i = 0; i < num; ++i) {
            coeff_[i] = dist_nbit_(gen_);
        }
    }

    void FillCoeffRandomRows(int num) {
        FillCoeffZero();
        for (int i = 0; i < num; ++i) {
            coeff_[i] = dist_nbit_(gen_);
        }
    }

    void FillCoeffZero() {
        FillCoeff(0);
    }

    void FillCoeffConstant() {
        tran_low_t c = dist_nbit_(gen_);
        FillCoeff(c);
    }

    void FillDcOnly() {
        FillCoeffZero();
        coeff_[0] = dist_nbit_(gen_);
    }

    void FillDcLargeNegative() {
        FillCoeffZero();
        // Generate a qcoeff which contains 512/-512 (0x0100/0xFE00) to catch
        // issues like BUG=883 where the constant being compared was incorrectly
        // initialized.
        coeff_[0] = -8191;
    }

    std::mt19937 gen_;
    std::uniform_int_distribution<> dist_nbit_;
    QuanTable *qtab_;
    tran_low_t *coeff_;
    QuantizeFunc quant_ref_;
    QuantizeFunc quant_test_;
    TxSize tx_size_;
    aom_bit_depth_t bd_;
};

TEST_P(QuantizeTest, ZeroInput) {
    FillCoeffZero();
    QuantizeRun(false);
}

TEST_P(QuantizeTest, LargeNegativeInput) {
    FillDcLargeNegative();
    QuantizeRun(false, 0, 1);
}

TEST_P(QuantizeTest, DcOnlyInput) {
    FillDcOnly();
    QuantizeRun(false, 0, 1);
}

TEST_P(QuantizeTest, RandomInput) {
    QuantizeRun(true, 0, kTestNum);
}

TEST_P(QuantizeTest, MultipleQ) {
    for (int q = 0; q < QINDEX_RANGE; ++q) {
        QuantizeRun(true, q, kTestNum);
    }
}

INSTANTIATE_TEST_CASE_P(
    AVX2, QuantizeTest,
    ::testing::Combine(::testing::Range(static_cast<int>(TX_4X4),
                                        static_cast<int>(TX_SIZES_ALL), 1),
                       ::testing::Values(static_cast<int>(AOM_BITS_8),
                                         static_cast<int>(AOM_BITS_10))));

}  // namespace
