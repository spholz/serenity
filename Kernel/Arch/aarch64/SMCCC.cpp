/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/SMCCC.h>

namespace Kernel::SMCCC {

enum class Conduit {
    SMC,
    HVC,
};

// XXX: Don't hardcode this
static Conduit s_conduit = Conduit::SMC;

Result32 call32(u32 function_id, u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4, u32 arg5, u32 arg6)
{
    // XXX: Add clobbers

    if (s_conduit == Conduit::SMC) {
        register u32 x0 asm("x0") = function_id;
        register u32 x1 asm("x1") = arg0;
        register u32 x2 asm("x2") = arg1;
        register u32 x3 asm("x3") = arg2;
        register u32 x4 asm("x4") = arg3;
        register u32 x5 asm("x5") = arg4;
        register u32 x6 asm("x6") = arg5;
        register u32 x7 asm("x7") = arg6;

        asm volatile(
            "smc #0\n"
            : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
            :
            : "memory");

        return {
            .w0 = x0,
            .w1 = x1,
            .w2 = x2,
            .w3 = x3,
            .w4 = x4,
            .w5 = x5,
            .w6 = x6,
            .w7 = x7,
        };
    }

    register u32 x0 asm("x0") = function_id;
    register u32 x1 asm("x1") = arg0;
    register u32 x2 asm("x2") = arg1;
    register u32 x3 asm("x3") = arg2;
    register u32 x4 asm("x4") = arg3;
    register u32 x5 asm("x5") = arg4;
    register u32 x6 asm("x6") = arg5;
    register u32 x7 asm("x7") = arg6;

    asm volatile(
        "hvc #0\n"
        : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4), "+r"(x5), "+r"(x6), "+r"(x7)
        :
        : "memory");

    return {
        .w0 = x0,
        .w1 = x1,
        .w2 = x2,
        .w3 = x3,
        .w4 = x4,
        .w5 = x5,
        .w6 = x6,
        .w7 = x7,
    };
}

}
