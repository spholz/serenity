/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/EFIPrekernel/DebugOutput.h>
#include <Kernel/EFIPrekernel/Runtime.h>

namespace Kernel {

[[noreturn]] void __panic(char const* file, unsigned int line, char const* function);

#define PANIC(...)                                                  \
    do {                                                            \
        ::Kernel::ucs2_dbgln(u"PREKERNEL PANIC! :^(");              \
        dbgln(__VA_ARGS__);                                         \
        ::Kernel::__panic(__FILE__, __LINE__, __PRETTY_FUNCTION__); \
    } while (0)

}
