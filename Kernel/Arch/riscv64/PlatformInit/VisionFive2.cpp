/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/riscv64/PlatformInit.h>

#include <Kernel/Boot/BootInfo.h>

namespace Kernel {

void visionfive2_platform_init(StringView)
{
    // VF2 U-Boot Framebuffer
    // The `| 0x4'0000'0000` is a quirk needed to avoid having the framebuffer cached.
    g_boot_info.boot_framebuffer.paddr = PhysicalAddress { 0xfe00'0000 | 0x4'0000'0000 };
    g_boot_info.boot_framebuffer.width = 1920;
    g_boot_info.boot_framebuffer.height = 1080;
    g_boot_info.boot_framebuffer.bpp = 32;
    g_boot_info.boot_framebuffer.pitch = g_boot_info.boot_framebuffer.width * (g_boot_info.boot_framebuffer.bpp / 8);
    g_boot_info.boot_framebuffer.type = BootFramebufferType::BGRx8888;
}

}
