/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/RPi/V3D/PageTable.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel::RPi::V3D {

ErrorOr<PageTable> PageTable::create()
{
    auto entries = TRY(Memory::allocate_dma_region_as_typed_array<u32 volatile>(PAGE_TABLE_ENTRY_COUNT, "V3D Page Table"sv, Memory::Region::Access::ReadWrite));
    return PageTable { move(entries) };
}

void PageTable::insert_entries_for_buffer(Badge<V3D>, GPUVirtualAddress gpu_vaddr, Memory::VMObject const& vmobject)
{
    VERIFY(static_cast<u64>(gpu_vaddr.value()) + vmobject.size() < 4 * GiB);

    auto gpu_vaddr_start_page_index = gpu_vaddr.value() / V3D_PAGE_SIZE;
    auto page_count = vmobject.size() / V3D_PAGE_SIZE;
    auto gpu_vaddr_end_page_index = gpu_vaddr_start_page_index + page_count;

    static_assert(PAGE_SIZE == V3D_PAGE_SIZE);

    for (size_t page_index = gpu_vaddr_start_page_index; page_index < gpu_vaddr_end_page_index; page_index++) {
        auto page_index_in_vmobject = page_index - gpu_vaddr_start_page_index;

        auto paddr = vmobject.physical_pages()[page_index_in_vmobject]->paddr();

        VERIFY(page_index < PAGE_TABLE_ENTRY_COUNT);
        m_entries[page_index] = (paddr.get() / V3D_PAGE_SIZE) | PAGE_TABLE_ENTRY_VALID | PAGE_TABLE_ENTRY_WRITABLE;
    }
}

void PageTable::remove_entries_for_buffer(Badge<V3D>, GPUVirtualAddress gpu_vaddr, Memory::VMObject const& vmobject)
{
    VERIFY(static_cast<u64>(gpu_vaddr.value()) + vmobject.size() < 4 * GiB);

    auto gpu_vaddr_start_page_index = gpu_vaddr.value() / V3D_PAGE_SIZE;
    auto page_count = vmobject.size() / V3D_PAGE_SIZE;
    auto gpu_vaddr_end_page_index = gpu_vaddr_start_page_index + page_count;

    static_assert(PAGE_SIZE == V3D_PAGE_SIZE);

    for (size_t page_index = gpu_vaddr_start_page_index; page_index < gpu_vaddr_end_page_index; page_index++) {
        VERIFY(page_index < PAGE_TABLE_ENTRY_COUNT);
        m_entries[page_index] = 0;
    }
}

PageTable::PageTable(Memory::TypedMapping<u32 volatile[]> entries)
    : m_entries(move(entries))
{
}

}
