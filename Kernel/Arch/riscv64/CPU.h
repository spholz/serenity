/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>

namespace Kernel {

void dbgln_without_mmu(StringView message);
[[noreturn]] void panic_without_mmu(StringView message);

namespace Memory {
void init_page_tables();
}
}
