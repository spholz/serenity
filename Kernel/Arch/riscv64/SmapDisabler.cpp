/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/SmapDisabler.h>

namespace Kernel {

SmapDisabler::SmapDisabler()
    : m_flags(0)
{
    asm volatile("csrs sstatus, %0" ::"r"(1 << 18));
}

SmapDisabler::~SmapDisabler()
{
    // FIXME: only disable if previously disabled
    asm volatile("csrc sstatus, %0" ::"r"(1 << 18));
}

}
