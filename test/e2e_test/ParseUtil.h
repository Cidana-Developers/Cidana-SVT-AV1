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
#include <string>
#include "aom/aom_image.h"

#define MAX_NUM_OP_POINTS 32

namespace svt_av1_e2e_tools {

/** SequenceHeaderParser is a class designed for parsing sequence header without
 * decode. It can recieve OBU data blocks and retrieve the parameter value by
 * name.
 */
class SequenceHeaderParser {
  public:
    SequenceHeaderParser() {
        profile_ = 0;
        sb_size_ = 0;
    }
    virtual ~SequenceHeaderParser() {
    }

  public:
    /** input OBU data to parser
     * @param obu_data the OBU data block buffer
     * @param size the size of OBU data block in bytes
     */
    void input_obu_data(const uint8_t* obu_data, const uint32_t size);
    /** get parameter value by its name
     * @param name the name of paramter in string
     * @return
     * std::string the value of the paramter in string format
     */
    std::string get_syntax_element(const std::string& name);

  private:
    uint32_t profile_; /**< profile paramter*/
    uint32_t sb_size_; /**< super block size paramter */
};
}  // namespace svt_av1_e2e_tools

#endif  // _PARSE_UTIL_H_
