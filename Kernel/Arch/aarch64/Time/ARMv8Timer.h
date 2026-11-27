/*
 * Copyright (c) 2024, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>
#include <Kernel/Library/NonnullLockRefPtr.h>
#include <Kernel/Time/HardwareTimer.h>

// https://developer.arm.com/-/media/Arm%20Developer%20Community/PDF/Learn%20the%20Architecture/Generic%20Timer.pdf

namespace Kernel {

class ARMv8Timer final : public HardwareTimer<IRQHandler> {
public:
    enum class Type {
        Virtual,
        Physical,
    };

    static ErrorOr<NonnullLockRefPtr<ARMv8Timer>> initialize(InterruptNumber interrupt_number, u32 frequency, Type type = Type::Virtual);

    static bool is_initialized();
    static ARMv8Timer& the();

    virtual HardwareTimerType timer_type() const override { return HardwareTimerType::ARMv8Timer; }
    virtual StringView model() const override
    {
        if (m_type == Type::Virtual)
            return "ARMv8 EL1 Virtual Timer"sv;

        if (m_type == Type::Physical)
            return "ARMv8 EL1 Physical Timer"sv;

        VERIFY_NOT_REACHED();
    }

    virtual size_t ticks_per_second() const override { return m_frequency; }

    virtual bool is_periodic() const override { return false; }
    virtual bool is_periodic_capable() const override { return false; }
    virtual void set_periodic() override { }
    virtual void set_non_periodic() override { }
    virtual void disable() override;

    virtual void reset_to_default_ticks_per_second() override { }
    virtual bool try_to_set_frequency(size_t frequency) override
    {
        if (!is_capable_of_frequency(frequency))
            return false;

        m_interrupt_frequency = frequency;
        m_interrupt_interval = m_frequency / frequency;

        dbgln("{} Timer frequency: {}, interrupt frequency: {}, delta counter value: {}", m_type == Type::Virtual ? "Virtual"sv : "Physical"sv, m_frequency, m_interrupt_frequency, m_interrupt_interval);

        start_timer(m_interrupt_interval);
        return true;
    }

    virtual bool is_capable_of_frequency(size_t frequency) const override
    {
        return frequency <= m_frequency;
    }

    virtual size_t calculate_nearest_possible_frequency(size_t frequency) const override
    {
        return min(frequency, m_frequency);
    }

    // FIXME: Share code with HPET::update_time
    u64 update_time(u64& seconds_since_boot, u32& ticks_this_second, bool query_only);

private:
    ARMv8Timer(InterruptNumber interrupt_number, u32 frequency, Type type);

    u64 current_ticks();
    void start_timer(u32 delta);

    // ^IRQHandler
    bool handle_irq() override;

    Function<void()> m_callback;
    u64 m_frequency { 0 };
    u64 m_interrupt_frequency { OPTIMAL_TICKS_PER_SECOND_RATE };
    u32 m_interrupt_interval { 0 };

    u64 m_main_counter_last_read { 0 };
    u64 m_main_counter_drift { 0 };

    Type m_type;
};

}
