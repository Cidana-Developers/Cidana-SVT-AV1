/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file SvtAv1EncApiTest.h
 *
 * @brief Define SvtAv1Context struct.
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/
#include "EbSvtAv1Enc.h"
#include "gtest/gtest.h"

namespace svt_av1_test {

typedef struct {
    EbComponentType* enc_handle;
    EbSvtAv1EncConfiguration enc_params;
} SvtAv1Context;

}  // namespace svt_av1_test
