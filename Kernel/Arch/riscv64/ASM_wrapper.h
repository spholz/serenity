/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/riscv64/Registers.h>

namespace Kernel::RiscV64::Asm {

inline void set_satp(FlatPtr satp)
{
    asm volatile("csrw satp, %[value]" ::[value] "r"(satp));
}

inline FlatPtr get_scause()
{
    uintptr_t scause;
    asm volatile("csrr %0, scause"
                 : "=r"(scause));
    return scause;
}

inline FlatPtr get_stval()
{
    uintptr_t stval;
    asm volatile("csrr %0, stval"
                 : "=r"(stval));
    return stval;
}

inline FlatPtr get_satp()
{
    uintptr_t satp;
    asm volatile("csrr %0, satp"
                 : "=r"(satp));
    return satp;
}

}
