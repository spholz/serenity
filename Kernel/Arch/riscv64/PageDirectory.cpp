/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <AK/Singleton.h>

#include <Kernel/API/POSIX/errno.h>
#include <Kernel/Arch/PageDirectory.h>
#include <Kernel/Arch/riscv64/ASM_wrapper.h>
#include <Kernel/Interrupts/InterruptDisabler.h>
#include <Kernel/Library/LockRefPtr.h>
#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Sections.h>

namespace Kernel::Memory {

struct SatpMap {
    SpinlockProtected<IntrusiveRedBlackTree<&PageDirectory::m_tree_node>, LockRank::None> map {};
};

static Singleton<SatpMap> s_satp_map;

void PageDirectory::register_page_directory(PageDirectory* directory)
{
    // dbgln("XXX PageDirectory::register_page_directory({:p}): directory->satp(): {:p}", directory, directory->satp());

    s_satp_map->map.with([&](auto& map) {
        map.insert(directory->satp(), *directory);
    });
}

ErrorOr<NonnullLockRefPtr<PageDirectory>> PageDirectory::try_create_for_userspace(Process& process)
{
    auto directory = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) PageDirectory));

    directory->m_process = &process;

    directory->m_directory_table = TRY(MM.allocate_physical_page());
    auto kernel_pd_index = (kernel_mapping_base >> 30) & 0x1ffu;
    for (size_t i = 0; i < kernel_pd_index; i++) {
        directory->m_directory_pages[i] = TRY(MM.allocate_physical_page());
    }

    // Share the top 1 GiB of kernel-only mappings (>=kernel_mapping_base)
    directory->m_directory_pages[kernel_pd_index] = MM.kernel_page_directory().m_directory_pages[kernel_pd_index];

    {
        InterruptDisabler disabler;
        auto& table = *(PageDirectoryPointerTable*)MM.quickmap_page(*directory->m_directory_table);
        for (size_t i = 0; i < sizeof(m_directory_pages) / sizeof(m_directory_pages[0]); i++) {
            if (directory->m_directory_pages[i]) {
                table.raw[i] = (FlatPtr)directory->m_directory_pages[i]->paddr().as_ptr();
            }
        }
        MM.unquickmap_page();
    }

    register_page_directory(directory);
    return directory;
}

LockRefPtr<PageDirectory> PageDirectory::find_current()
{
    return s_satp_map->map.with([&](auto& map) {
        return map.find(RISCV64::Asm::get_satp());
    });
}

void activate_kernel_page_directory(PageDirectory const& page_directory)
{
    dbgln("XXX activate_kernel_page_directory({:p}): page_directory.satp(): {:p}", &page_directory, page_directory.satp());

    FlatPtr const satp_val = page_directory.satp();
    RISCV64::Asm::set_satp(satp_val);
    Processor::flush_entire_tlb_local();
}

void activate_page_directory(PageDirectory const& page_directory, Thread* current_thread)
{
    dbgln("XXX activate_page_directory({:p}, {}): page_directory.satp(): {:p}", &page_directory, *current_thread, page_directory.satp());

    FlatPtr const satp_val = page_directory.satp();
    current_thread->regs().satp = satp_val;
    RISCV64::Asm::set_satp(satp_val);
    Processor::flush_entire_tlb_local();
}

UNMAP_AFTER_INIT NonnullLockRefPtr<PageDirectory> PageDirectory::must_create_kernel_page_directory()
{
    return adopt_lock_ref_if_nonnull(new (nothrow) PageDirectory).release_nonnull();
}

UNMAP_AFTER_INIT void PageDirectory::allocate_kernel_directory()
{
    dmesgln("MM: boot_pdpt @ {}", boot_pdpt);
    dmesgln("MM: boot_pd0 @ {}", boot_pd0);
    dmesgln("MM: boot_pd_kernel @ {}", boot_pd_kernel);
    m_directory_table = PhysicalPage::create(boot_pdpt, MayReturnToFreeList::No);
    m_directory_pages[0] = PhysicalPage::create(boot_pd0, MayReturnToFreeList::No);
    m_directory_pages[(kernel_mapping_base >> 30) & 0x1ff] = PhysicalPage::create(boot_pd_kernel, MayReturnToFreeList::No);
}

}
