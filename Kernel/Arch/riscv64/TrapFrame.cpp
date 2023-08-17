/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Processor.h>
#include <Kernel/Arch/riscv64/TrapFrame.h>

namespace Kernel {

extern "C" void exit_trap(TrapFrame* trap)
{
    return Processor::current().exit_trap(*trap);
}

}
