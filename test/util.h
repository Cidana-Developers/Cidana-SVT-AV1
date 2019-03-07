/*
 * Copyright(c) 2019 Intel Corporation
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

#ifndef _TEST_UTIL_H_
#define _TEST_UTIL_H_

#include <math.h>
#include <stdio.h>
#include "gtest/gtest.h"

// Macros
#ifndef PI
#define PI 3.141592653589793238462643383279502884
#endif

#ifndef TEST_GET_PARAM
#define TEST_GET_PARAM(k) std::tr1::get<k>(GetParam())
#endif

#endif  // _TEST_UTIL_H_
