/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Buffer.h"

#include <LibCore/System.h>
#include <sys/mman.h>

#include <Kernel/API/V3D.h>

extern int g_v3d_fd;

BufferObject::~BufferObject()
{
    if (m_handle == 0xffff'ffff)
        return; // XXX: Remove once no-arg constructor is removed.

    if (m_mmap_address != nullptr) {
        if (auto result = Core::System::munmap(m_mmap_address, m_size); result.is_error())
            dbgln("~BufferObject(): munmap({}, {:#x}) failed: {}", m_mmap_address, m_size, result.release_error());
    }

    if (auto result = Core::System::ioctl(g_v3d_fd, V3D_FREE_BUFFER, m_handle); result.is_error())
        dbgln("~BufferObject(): ioctl({}, V3D_FREE_BUFFER, {}) failed: {}", g_v3d_fd, m_handle, result.release_error());
}

ErrorOr<BufferObject> BufferObject::create(u32 size)
{
    V3DBuffer buffer = {
        .size = size,

        // Will be filled by the kernel
        .id = 0,
        .address = 0,
        .mmap_offset = 0,
    };

    TRY(Core::System::ioctl(g_v3d_fd, V3D_ALLOCATE_BUFFER, &buffer));

    return BufferObject(buffer.id, buffer.size, buffer.address, static_cast<off_t>(buffer.mmap_offset));
}

ErrorOr<void*> BufferObject::map()
{
    // Each BufferObject should only be mapped once!
    VERIFY(m_mmap_address == nullptr);

    m_mmap_address = TRY(Core::System::mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_v3d_fd, m_mmap_offset, 0, "V3D Buffer"sv));
    return m_mmap_address;
}
