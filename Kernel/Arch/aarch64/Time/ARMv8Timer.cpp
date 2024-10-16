/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <Kernel/Arch/aarch64/Time/ARMv8Timer.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>

namespace Kernel {

ARMv8Timer::ARMv8Timer()
    : HardwareTimer(30) // TODO: Don't hardcode this
{
    // TODO: Handle dt clock-frequency property
    m_frequency = Aarch64::CNTFRQ_EL0::read().ClockFrequency;
    VERIFY(m_frequency != 0);

    m_interrupt_interval = m_frequency / OPTIMAL_TICKS_PER_SECOND_RATE;

    start_timer(m_interrupt_interval);
}

NonnullLockRefPtr<ARMv8Timer> ARMv8Timer::initialize()
{
    auto timer = adopt_lock_ref(*new ARMv8Timer);

    // Enable interrupts
    auto ctl = Aarch64::CNTP_CTL_EL0::read();
    ctl.IMASK = 0;
    ctl.ENABLE = 1;
    Aarch64::CNTP_CTL_EL0::write(ctl);

    timer->enable_irq();

    return timer;
}

u64 ARMv8Timer::current_ticks()
{
    return Aarch64::CNTPCT_EL0::read().PhysicalCount;
}

bool ARMv8Timer::handle_irq()
{
    HardwareTimer::handle_irq();

    if (Aarch64::CNTP_CTL_EL0::read().ISTATUS == 0)
        return false;

    start_timer(m_interrupt_interval);

    return true;
}

u64 ARMv8Timer::update_time(u64& seconds_since_boot, u32& ticks_this_second, bool query_only)
{
    // Should only be called by the time keeper interrupt handler!
    u64 current_value = current_ticks();
    u64 delta_ticks = m_main_counter_drift;
    if (current_value >= m_main_counter_last_read) {
        delta_ticks += current_value - m_main_counter_last_read;
    } else {
        // the counter wrapped around
        delta_ticks += (NumericLimits<u64>::max() - m_main_counter_last_read + 1) + current_value;
    }

    u64 ticks_since_last_second = (u64)ticks_this_second + delta_ticks;
    auto frequency = ticks_per_second();
    seconds_since_boot += ticks_since_last_second / frequency;
    ticks_this_second = ticks_since_last_second % frequency;

    if (!query_only) {
        m_main_counter_drift = 0;
        m_main_counter_last_read = current_value;
    }

    // Return the time passed (in ns) since last time update_time was called
    return (delta_ticks * 1000000000ull) / frequency;
}

void ARMv8Timer::start_timer(u32 delta)
{
    Aarch64::CNTP_TVAL_EL0::write(Aarch64::CNTP_TVAL_EL0 {
        .TimerValue = delta,
    });
}

}
