/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/BitCast.h>
#include <AK/Error.h>
#include <AK/Types.h>

namespace Kernel::SMCCC {

enum class ReturnCode : u32 {
    SUCCESS = 0,
    NOT_SUPPORTED = bit_cast<u32>(-1),
    INVALID_PARAMETERS = bit_cast<u32>(-2),
    DENIED = bit_cast<u32>(-3),
};

struct Result32 {
    u32 w0;
    u32 w1;
    u32 w2;
    u32 w3;
    u32 w4;
    u32 w5;
    u32 w6;
    u32 w7;
};

template<typename T>
using ReturnCodeOr = ErrorOr<T, ReturnCode>;

Result32 call32(u32 function_id, u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6);

}
