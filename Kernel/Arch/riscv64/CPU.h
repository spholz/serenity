/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Prekernel/Prekernel.h>

#include <AK/Platform.h>
VALIDATE_IS_RISCV64()

namespace Kernel {

extern BootInfo s_boot_info;

bool is_vf2();

}
