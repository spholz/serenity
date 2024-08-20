/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Assertions.h>
#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/Processor.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>

namespace Kernel {

void microseconds_delay(u32 microseconds)
{
    // TODO: Handle dt clock-frequency property
    auto frequency = Aarch64::CNTFRQ_EL0::read().ClockFrequency;
    VERIFY(frequency != 0);

    u64 const start = Aarch64::CNTPCT_EL0::read().PhysicalCount;
    u64 const delta = (static_cast<u64>(microseconds) * frequency) / 1'000'000ull;

    while ((Aarch64::CNTPCT_EL0::read().PhysicalCount - start) < delta)
        Processor::pause();
}

}
