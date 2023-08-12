/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/StringView.h>
#include <AK/Types.h>
#include <Kernel/Arch/riscv64/CPU.h>
#include <Kernel/Arch/riscv64/SBI.h>
#include <Kernel/Sections.h>
#include <Kernel/kstdio.h>

namespace Kernel {

extern "C" [[noreturn]] void init();

extern "C" [[noreturn]] void pre_init(FlatPtr mhartid, FlatPtr fdt_phys_addr);
extern "C" [[noreturn]] void pre_init(FlatPtr mhartid, FlatPtr fdt_phys_addr)
{
    (void)mhartid;
    (void)fdt_phys_addr;

    asm volatile(
        "la t0, trap_handler_nommu \n"
        "csrw stvec, t0 \n");

    // dbgln_without_mmu("Memory::init_page_tables()"sv);
    Memory::init_page_tables();
    // dbgln_without_mmu("Memory::init_page_tables() done"sv);

    // At this point the MMU is enabled, physical memory is identity mapped,
    // and the kernel is also mapped into higher virtual memory. However we are still executing
    // from the physical memory address, so we have to jump to the kernel in high memory. We also need to
    // switch the stack pointer to high memory, such that we can unmap the identity mapping.

    // Continue execution at high virtual address, by using an absolute jump.
    asm volatile(
        "    ld t0, 1f \n"
        "    jr t0 \n"
        "1:  .dword 2f \n"
        "2:  \n" ::
            : "t0");

    // Add kernel_mapping_base to the stack pointer and to the global pointer, such that it is also using the mapping
    // in high virtual memory.
    asm volatile(
        "    mv t0, %[base] \n"
        "    add sp, sp, t0 \n"
        ".option push \n"
        ".option norelax \n"
        "    add gp, gp, t0 \n"
        ".option pop \n" ::[base] "r"(physical_to_virtual_offset)
        : "t0");

    // We can now unmap the identity map as everything is running in high virtual memory at this point.
    // Memory::unmap_identity_map();

    // Set the Supervisor Trap Vector Base Address Register (stvec) to the trap handler function
    asm volatile(
        "la t0, asm_trap_handler \n"
        "csrw stvec, t0 \n" ::
            : "t0");

    // Clear the frame pointer (fp) and return address register (ra) to make sure the kernel cannot backtrace
    // into this code, and jump to actual init function in the kernel.
    // dbgln_without_mmu("calling init()!"sv);
    asm volatile(
        "mv fp, zero \n"
        "mv ra, zero \n"
        "tail init \n");

    VERIFY_NOT_REACHED();
}

void dbgln_without_mmu(StringView message)
{
    for (auto c : message)
        (void)SBI::Legacy::console_putchar(c);
    (void)SBI::Legacy::console_putchar('\n');
}

[[noreturn]] void panic_without_mmu(StringView message)
{
    dbgln_without_mmu("PANIC!\n"sv);
    dbgln_without_mmu(message);

    for (;;) {
        asm volatile("wfi");
        asm volatile("pause");
    }
}

}
