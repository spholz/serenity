/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <AK/Vector.h>

#include "Buffer.h"
#include "ControlList.h"

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

struct Job {
    Vector<u32> bo_handles;
    BufferObject tile_alloc_memory_bo;
    BufferObject tile_state_data_array_bo;

    ControlList binner_control_list;
    ControlList render_control_list;
};

#ifdef KERNEL
}
#endif
