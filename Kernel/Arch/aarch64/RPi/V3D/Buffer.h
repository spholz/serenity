/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

struct BufferObject {
    u32 handle;
    u32 size;
    u32 offset;
};

BufferObject create_buffer_object(u32 size);
void* map_buffer_object(BufferObject const& bo);

#ifdef KERNEL
}
#endif
