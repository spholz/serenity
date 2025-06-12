/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Buffer.h"

#include <AK/Assertions.h>
#include <Kernel/API/Ioctl.h>
#include <Kernel/API/V3D.h>
#include <LibCore/System.h>
#include <sys/mman.h>

extern int g_v3d_fd;

BufferObject create_buffer_object(u32 size)
{
    V3DBuffer buffer = {
        .size = size,

        // Will be filled by the kernel
        .id = 0,
        .address = 0,
        .mmap_offset = 0,
    };

    MUST(Core::System::ioctl(g_v3d_fd, V3D_CREATE_BUFFER, &buffer));

    return BufferObject {
        .handle = buffer.id,
        .size = buffer.size,
        .offset = buffer.address,
        .mmap_offset = buffer.mmap_offset,
    };
}

void* map_buffer_object(BufferObject const& bo)
{
    return MUST(Core::System::mmap(nullptr, bo.size, PROT_READ | PROT_WRITE, MAP_SHARED, g_v3d_fd, static_cast<off_t>(bo.mmap_offset)));
}
