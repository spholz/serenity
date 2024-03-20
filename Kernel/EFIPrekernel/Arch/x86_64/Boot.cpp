/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

#include <Kernel/Sections.h>

#include <Kernel/EFIPrekernel/Arch/Boot.h>
#include <Kernel/EFIPrekernel/Arch/MMU.h>
#include <Kernel/EFIPrekernel/DebugOutput.h>
#include <Kernel/EFIPrekernel/EFIPrekernel.h>
#include <Kernel/EFIPrekernel/Globals.h>
#include <Kernel/EFIPrekernel/Panic.h>

namespace Kernel {

// This function has to fit into one page as it will be identity mapped.
[[gnu::aligned(PAGE_SIZE)]] [[noreturn]] static void enter_kernel_helper(FlatPtr cr3, FlatPtr kernel_entry, FlatPtr kernel_sp, FlatPtr boot_info_vaddr)
{
    register FlatPtr rdi asm("rdi") = boot_info_vaddr;
    register FlatPtr rsp asm("rsp") = kernel_sp;
    asm volatile(
        R"(
        movq %[cr3], %%cr3

        mov $0, %%ax
        mov %%ax, %%ss
        mov %%ax, %%ds
        mov %%ax, %%es
        mov %%ax, %%fs
        mov %%ax, %%gs

        pushq $0
        jmp *%[kernel_entry]
        )"
        :
        : "r"(rdi), "r"(rsp), [cr3] "r"(cr3), [kernel_sp] "r"(kernel_sp), [kernel_entry] "r"(kernel_entry)
        : "rax");

    __builtin_unreachable();
}

// FIXME: Rename me?
void arch_prepare_boot(void* root_page_table, BootInfo& boot_info)
{
    (void)root_page_table;
    (void)boot_info;

    // TODO: Find ACPI table

    // FIXME: This leaks < (page table levels) pages, since all active allocations after ExitBootServices are currently eternal.
    //        We could theoretically reclaim them in the kernel.
    // NOTE: If this map_pages ever fails, the kernel vaddr range is inside our (physical) prekernel range
    if (auto result = Memory::map_pages(root_page_table, bit_cast<FlatPtr>(&enter_kernel_helper), bit_cast<PhysicalPtr>(&enter_kernel_helper), 1, Memory::Access::Read | Memory::Access::Execute); result.is_error())
        PANIC("Failed to identity map the enter_kernel_helper function: {}", result.release_error());

    boot_info.boot_method_specific.efi.bootstrap_page_vaddr = VirtualAddress { bit_cast<FlatPtr>(&enter_kernel_helper) };

    auto maybe_kernel_page_directory = Memory::get_or_insert_page_table(root_page_table, boot_info.kernel_mapping_base, 1);
    if (maybe_kernel_page_directory.is_error())
        PANIC("Could not find the kernel page directory: {}", maybe_kernel_page_directory.release_error());

    auto maybe_kernel_pdpt = Memory::get_or_insert_page_table(root_page_table, boot_info.kernel_mapping_base, 2);
    if (maybe_kernel_pdpt.is_error())
        PANIC("Could not find the kernel page directory pointer table: {}", maybe_kernel_pdpt.release_error());

    boot_info.boot_pml4t = PhysicalAddress { bit_cast<PhysicalPtr>(root_page_table) };
    boot_info.boot_pdpt = PhysicalAddress { bit_cast<PhysicalPtr>(maybe_kernel_pdpt.value()) };
    boot_info.boot_pd_kernel = PhysicalAddress { bit_cast<PhysicalPtr>(maybe_kernel_page_directory.value()) };

    auto kernel_pt1024_base = boot_info.kernel_mapping_base + KERNEL_PT1024_OFFSET;

    auto maybe_quickmap_page_table_paddr = Memory::get_or_insert_page_table(root_page_table, kernel_pt1024_base, 0, true);
    if (maybe_quickmap_page_table_paddr.is_error())
        PANIC("Failed to insert the quickmap page table: {}", maybe_quickmap_page_table_paddr.release_error());

    boot_info.boot_pd_kernel_pt1023 = bit_cast<Memory::PageTableEntry*>(KERNEL_MAPPING_BASE + 0x200000 - PAGE_SIZE);

    if (auto result = Memory::map_pages(root_page_table, bit_cast<FlatPtr>(boot_info.boot_pd_kernel_pt1023), bit_cast<PhysicalPtr>(maybe_quickmap_page_table_paddr.value()), 1, Memory::Access::Read | Memory::Access::Write); result.is_error())
        PANIC("Failed to map the quickmap page table: {}", result.release_error());
}

[[noreturn]] void arch_enter_kernel(void* root_page_table, FlatPtr kernel_entry_vaddr, FlatPtr kernel_stack_pointer, FlatPtr boot_info_vaddr)
{
    FlatPtr cr3 = bit_cast<FlatPtr>(root_page_table);

    enter_kernel_helper(bit_cast<FlatPtr>(cr3), kernel_entry_vaddr, kernel_stack_pointer, boot_info_vaddr);
}

}
