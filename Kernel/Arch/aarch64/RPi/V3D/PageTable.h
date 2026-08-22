/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Memory/VMObject.h>

namespace Kernel::RPi::V3D {

class V3D;

class PageTable {
public:
    static ErrorOr<PageTable> create();

    void insert_entries_for_buffer(Badge<V3D>, u32 gpu_vaddr, Memory::VMObject const&);
    void remove_entries_for_buffer(Badge<V3D>, u32 gpu_vaddr, Memory::VMObject const&);

    PhysicalAddress physical_address() const { return m_entries.paddr; }

private:
    static constexpr size_t PAGE_TABLE_ENTRY_COUNT = (4uz * GiB) / (4uz * KiB);

    static constexpr u32 PAGE_TABLE_ENTRY_WRITABLE = 1u << 29;
    static constexpr u32 PAGE_TABLE_ENTRY_VALID = 1u << 28;

    PageTable(Memory::TypedMapping<u32 volatile[]>);

    Memory::TypedMapping<u32 volatile[]> m_entries;
};

}
