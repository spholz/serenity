/*
 * Copyright (c) 2022, Liav A. <liavalb@hotmail.co.il>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/DebugOutput.h>
#include <Kernel/Arch/aarch64/RPi/UART.h>

bool volatile g_semihosting_debug_output_enabled { false };

namespace Kernel {

void debug_output(char ch)
{
    if (g_semihosting_debug_output_enabled) {
        char const c = ch;

        register FlatPtr x0 asm("x0");
        register u32 const w0 asm("w0") = 0x03;
        register FlatPtr const x1 asm("x1") = bit_cast<FlatPtr>(&c);

        asm volatile(
            "hlt #0xf000\n"
            : "=r"(x0)
            : "r"(w0), "r"(x1)
            : "memory");

        return;
    }

    if (!Memory::MemoryManager::is_initialized())
        return;

    RPi::UART::the().send(ch);
}

}
