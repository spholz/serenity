/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Memory/Region.h>

namespace Kernel::Memory {

class ContiguousDMABuffer {
public:
    ContiguousDMABuffer(NonnullOwnPtr<Memory::Region> region, u64 bus_address, bool is_cache_coherent)
        : m_region(move(region))
        , m_bus_address(bus_address)
        , m_is_cache_coherent(is_cache_coherent)
    {
    }

    u64 bus_address() const { return m_bus_address; }
    VirtualAddress virtual_address() const { return m_region->vaddr(); }
    size_t size() const { return m_region->size(); }

private:
    NonnullOwnPtr<Memory::Region> m_region;
    u64 m_bus_address;
    bool m_is_cache_coherent { false };
};

}
