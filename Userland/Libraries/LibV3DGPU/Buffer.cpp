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

Buffer::~Buffer()
{
    if (m_size == 0)
        return; // XXX: Remove once no-arg constructor is removed.

    if (m_mmap_address != nullptr) {
        if (auto result = Core::System::munmap(m_mmap_address, m_size); result.is_error())
            dbgln("~Buffer(): munmap({}, {:#x}) failed: {}", m_mmap_address, m_size, result.release_error());
    }

    if (auto result = Core::System::ioctl(g_v3d_fd, V3D_FREE_BUFFER, m_gpu_virtual_address); result.is_error()) {
        VERIFY_NOT_REACHED();
        dbgln("~Buffer(): ioctl({}, V3D_FREE_BUFFER, {:#x}) failed: {}", g_v3d_fd, m_gpu_virtual_address, result.release_error());
    }
}

ErrorOr<Buffer> Buffer::create(u32 size)
{
    V3DBuffer buffer = {
        .size = size,

        // Will be filled by the kernel.
        .gpu_virtual_address = 0,
    };

    TRY(Core::System::ioctl(g_v3d_fd, V3D_ALLOCATE_BUFFER, &buffer));

    VERIFY(buffer.gpu_virtual_address != 0);

    return Buffer(buffer.gpu_virtual_address, buffer.size);
}

ErrorOr<void*> Buffer::map()
{
    // Each Buffer should only be mapped once!
    VERIFY(m_mmap_address == nullptr);

    m_mmap_address = TRY(Core::System::mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, g_v3d_fd, m_gpu_virtual_address, 0, "V3D Buffer"sv));
    return m_mmap_address;
}
