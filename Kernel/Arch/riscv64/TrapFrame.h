/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/RegisterState.h>

namespace Kernel {

struct TrapFrame {
    TrapFrame* next_trap;
    RegisterState* regs;

    TrapFrame() = delete;
    TrapFrame(TrapFrame const&) = delete;
    TrapFrame(TrapFrame&&) = delete;
    TrapFrame& operator=(TrapFrame const&) = delete;
    TrapFrame& operator=(TrapFrame&&) = delete;
};

#define TRAP_FRAME_SIZE (2 * 8)
static_assert(AssertSize<TrapFrame, TRAP_FRAME_SIZE>());

}
