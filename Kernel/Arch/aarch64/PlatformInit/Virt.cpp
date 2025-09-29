/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/NeverDestroyed.h>
#include <Kernel/Arch/aarch64/PlatformInit.h>

#include <Kernel/Arch/aarch64/DebugOutput.h>
#include <Kernel/Arch/aarch64/Serial/PL011.h>

namespace Kernel {

void virt_platform_init(StringView)
{
#if 0
    // We have to use a raw pointer here because this variable will be set before global constructors are called.
    static PL011* s_debug_console_uart;

    static DebugConsole const s_debug_console {
        .write_character = [](char character) {
            s_debug_console_uart->send(character);
        },
    };

    s_debug_console_uart = MUST(PL011::initialize(PhysicalAddress { 0x0900'0000 })).leak_ptr();
    set_debug_console(&s_debug_console);
#else
    // We have to use a raw pointer here because this variable will be set before global constructors are called.
    static NeverDestroyed<Optional<Memory::TypedMapping<u8 volatile>>> s_16550_thr;

    static DebugConsole const s_debug_console {
        .write_character = [](char character) {
            ***s_16550_thr = character;
        },
    };

    *s_16550_thr = MUST(Memory::map_typed_writable<u8 volatile>(PhysicalAddress { 0x3f8 }));
    set_debug_console(&s_debug_console);
#endif
}

}
