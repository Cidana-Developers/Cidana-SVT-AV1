/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file AdaptiveScanTest.cc
 *
 * @brief Unit test for adaptive scan:
 *
 * @author Cidana-Wenyao
 *
 ******************************************************************************/
#include "gtest/gtest.h"
#include <stdlib.h>
// workaround to eliminate the compiling warning on linux
// The macro will conflict with definition in gtest.h
#ifdef __USE_GNU
#undef __USE_GNU  // defined in EbThreads.h
#endif
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE  // defined in EbThreads.h
#endif
#include "EbDefinitions.h"
#include "EbTransforms.h"
#include "TxfmCommon.h"
#include "EbCabacContextModel.h"  // use tx_type_to_class
static bool valid_scan(const int16_t *scan, const int16_t *iscan, int si,
                       int expected_pos) {
    if (scan[si] != expected_pos || iscan[expected_pos] != si) {
        return false;
    } else
        return true;
}

TEST(AdaptiveScanTest, table_test) {
    for (int tx_size = TX_4X4; tx_size < TX_SIZES_ALL; ++tx_size) {
        const int txb_height = get_txb_high((TxSize)tx_size);
        const int txb_width = get_txb_wide((TxSize)tx_size);
        const int bwl = get_txb_bwl((TxSize)tx_size);

        for (int tx_type = DCT_DCT; tx_type < TX_TYPES; ++tx_type) {
            if (!is_txfm_allowed((TxType)tx_type, (TxSize)tx_size))
                continue;

            TX_CLASS tx_class = tx_type_to_class[tx_type];
            const SCAN_ORDER *scan_order = &av1_scan_orders[tx_size][tx_type];
            const int16_t *scan = scan_order->scan;
            const int16_t *iscan = scan_order->iscan;
            if (tx_class == TX_CLASS_VERT) {
                // vertical transform will scan the rows horizontally,
                // so si is the same with pos
                // process horizontal scan first
                int si = 0;
                for (int row = 0; row < txb_height; ++row) {
                    for (int col = 0; col < txb_width; ++col) {
                        // convert to block pos and increase it
                        const int pos = scan[si];
                        ASSERT_EQ(pos, si)
                            << "tx_size " << tx_size << " tx_type " << tx_type
                            << " scan failed";
                        ASSERT_EQ(iscan[pos], si)
                            << "tx_size " << tx_size << " tx_type " << tx_type
                            << " iscan failed";
                        ++si;
                    }
                }
            } else if (tx_class == TX_CLASS_HORIZ) {
                // horizontal transform will scan vertically.
                int si = 0;
                for (int col = 0; col < txb_width; ++col) {
                    for (int row = 0; row < txb_height; ++row) {
                        const int pos = scan[si];
                        ASSERT_EQ(pos, col + (row << bwl))
                            << "tx_size " << tx_size << " tx_type " << tx_type
                            << " scan failed";
                        ASSERT_EQ(iscan[pos], si)
                            << "tx_size " << tx_size << " tx_type " << tx_type
                            << " iscan failed";
                        ++si;
                    }
                }
            } else if (txb_width < txb_height) {
                // cols are smaller than rows
                const int dim = txb_width + txb_height - 1;
                int si = 0;
                for (int i = 0; i < dim; ++i) {
                    for (int row = 0; row < txb_height; ++row) {
                        int col = i - row;
                        if (col >= 0 && col < txb_width) {
                            const int pos = scan[si];
                            ASSERT_EQ(pos, col + (row << bwl))
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " scan failed";
                            ASSERT_EQ(iscan[pos], si)
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " iscan failed";
                            ++si;
                        }
                    }
                }
            } else if (txb_width > txb_height) {
                // col diag
                const int dim = txb_width + txb_height - 1;
                int si = 0;
                for (int i = 0; i < dim; ++i) {
                    for (int col = 0; col < txb_width; ++col) {
                        int row = i - col;
                        if (row >= 0 && row < txb_height) {
                            const int pos = scan[si];
                            ASSERT_EQ(pos, col + (row << bwl))
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " scan failed";
                            ASSERT_EQ(iscan[pos], si)
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " iscan failed";
                            ++si;
                        }
                    }
                }
            } else {
                // zig-zag scan
                const int dim = txb_width + txb_height - 1;
                int swap = 1;
                int si = 0;
                for (int i = 0; i < dim; ++i) {
                    swap ^= 1;
                    for (int j = 0; j < txb_width; ++j) {
                        const int col = swap ? i - j : j;
                        const int row = swap ? j : i - j;
                        if (i - j >= 0 && i - j < txb_width) {
                            const int pos = scan[si];
                            ASSERT_EQ(col + (row << bwl), pos)
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " scan failed";
                            ASSERT_EQ(si, iscan[pos])
                                << "tx_size " << tx_size << " tx_type "
                                << tx_type << " iscan failed";
                            ++si;
                        }
                    }
                }
            }
        }
    }
}
