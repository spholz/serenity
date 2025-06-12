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

struct Job;

Job run_clear_color(FlatPtr target_buffer_address, size_t target_buffer_width, size_t target_buffer_height, size_t target_buffer_pitch);

#ifdef KERNEL
}
#endif
