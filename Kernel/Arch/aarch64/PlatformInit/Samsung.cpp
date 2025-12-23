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

}
