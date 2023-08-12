/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Singleton.h>

#include <Kernel/Arch/DebugOutput.h>
#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/riscv64/SBI.h>
#include <Kernel/Bus/PCI/Initializer.h>
#include <Kernel/Sections.h>
#include <Kernel/Tasks/Process.h>
#include <Kernel/kstdio.h>

namespace Kernel {

void microseconds_delay(u32 microseconds)
{
    (void)microseconds;
    // for (u32 i = 0; i < microseconds; i++)
    //     asm volatile("");
}

void debug_output(char ch)
{
    (void)SBI::Legacy::console_putchar(ch);
}

}

// void set_serial_debug_enabled(bool)
// {
// }

namespace Kernel::ACPI::StaticParsing {
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations();
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations()
{
    // FIXME: Implement finding RSDP for riscv64.
    return Optional<PhysicalAddress> {};
}
}
