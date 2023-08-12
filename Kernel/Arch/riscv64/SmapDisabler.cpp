/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/SmapDisabler.h>

namespace Kernel {

SmapDisabler::SmapDisabler()
    : m_flags(0)
{
}

SmapDisabler::~SmapDisabler() = default;

}
