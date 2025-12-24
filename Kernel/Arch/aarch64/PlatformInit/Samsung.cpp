/*
 * Copyright (c) 2024, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/NeverDestroyed.h>
#include <Kernel/Arch/aarch64/PlatformInit.h>

#include <Kernel/Arch/aarch64/DebugOutput.h>
#include <Kernel/Arch/aarch64/Serial/PL011.h>

namespace Kernel {

void samsung_a70q_platform_init(StringView)
{
    // Unforunately no easily accessible UART :^(

    // ... but we do have a framebuffer from the bootloader!
    g_boot_info.boot_framebuffer = {
        .paddr = PhysicalAddress { 0x9c000000 },
        .pitch = 1080uz * 4,
        .width = 1080,
        .height = 2400,
        .bpp = 32,
        .type = BootFramebufferType::BGRx8888,
    };
}

void samsung_xcover4lte_platform_init(StringView)
{
    static NeverDestroyed<Optional<Memory::TypedMapping<u8 volatile>>> s_uart;

    static DebugConsole const s_debug_console {
        .write_character = [](char character) {
            u8 volatile* uart_tx = (**s_uart).ptr() + 0x20;
            u32 volatile* uart_fifo_status = reinterpret_cast<u32 volatile*>((**s_uart).ptr() + 0x18);

            while ((*uart_fifo_status & (1 << 24)) != 0)
                Processor::pause();

            *uart_tx = character;
        },
    };

    *s_uart = MUST(Memory::map_typed_writable<u8 volatile>(PhysicalAddress { 0x13820000 }));
    set_debug_console(&s_debug_console);

    // The bootloader keeps the framebuffer active at that address.
    // (taken from the linux cmdline "s3cfb.bootloaderfb=0x67000000" arg)
    g_boot_info.boot_framebuffer = {
        .paddr = PhysicalAddress { 0x67000000 },
        .pitch = 720uz * 4,
        .width = 720,
        .height = 1280,
        .bpp = 32,
        .type = BootFramebufferType::BGRx8888,
    };
}

}
