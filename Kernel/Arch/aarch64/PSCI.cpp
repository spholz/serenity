/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/BitCast.h>
#include <Kernel/Arch/aarch64/PSCI.h>
#include <Kernel/Arch/aarch64/SMCCC.h>

// https://developer.arm.com/documentation/den0022/latest/

namespace Kernel::PSCI {

// 5.2.1 Register usage in arguments and return values
static u32 call32(u32 function_id, u32 arg0, u32 arg1, u32 arg2)
{
    return SMCCC::call32(function_id, arg0, arg1, arg2, 0, 0, 0, 0).w0;
}

Version get_version()
{
    return bit_cast<Version>(call32(0x8400'0000, 0, 0, 0));
}

ReturnErrorCodeOr<u32> get_features(u32 psci_func_id)
{
    auto result = call32(0x8400'000a, psci_func_id, 0, 0);
    if (bit_cast<i32>(result) < 0)
        return static_cast<ReturnErrorCode>(result);
    return result;
}

}
