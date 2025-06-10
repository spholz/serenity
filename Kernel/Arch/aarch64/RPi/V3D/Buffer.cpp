/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Buffer.h"

#include <AK/NeverDestroyed.h>
#include <Kernel/Memory/MemoryManager.h>

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

static NeverDestroyed<Vector<NonnullOwnPtr<Memory::Region>>> g_v3d_buffer_regions;

BufferObject create_buffer_object(u32 size)
{
    auto buffer_region = MUST(MM.allocate_dma_buffer_pages(16 * KiB, "V3D Binner Control List"sv, Memory::Region::Access::ReadWrite));
    auto paddr = buffer_region->physical_page(0)->paddr().get();

    g_v3d_buffer_regions->append(move(buffer_region));

    return BufferObject {
        .handle = static_cast<u32>(g_v3d_buffer_regions->size() - 1),
        .size = size,
        .offset = static_cast<u32>(paddr),
    };
}

void* map_buffer_object(BufferObject const& bo)
{
    auto& buffer_region = (*g_v3d_buffer_regions)[bo.handle];
    return buffer_region->vaddr().as_ptr();
}

#ifdef KERNEL
}
#endif
