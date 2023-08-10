#include <AK/Format.h>
#include <AK/NeverDestroyed.h>
#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Arch/riscv64/SBI.h>
#include <Kernel/Arch/riscv64/Timer.h>

namespace Kernel::RiscV {

Timer::Timer()
    : HardwareTimer(5)
{
    m_frequency = 3000000;

    // set_interrupt_interval_usec(m_frequency / OPTIMAL_TICKS_PER_SECOND_RATE);
    set_interrupt_interval_usec(m_frequency);
    enable_interrupt_mode();
}

Timer::~Timer() = default;

NonnullLockRefPtr<Timer> Timer::initialize()
{
    return adopt_lock_ref(*new Timer);
}

u64 Timer::microseconds_since_boot()
{
    return RiscV64::rdtime();
}

bool Timer::handle_irq(RegisterState const& regs)
{
    auto result = HardwareTimer::handle_irq(regs);

    set_compare(microseconds_since_boot() + m_interrupt_interval);
    clear_interrupt();

    return result;
}

u64 Timer::update_time(u64& seconds_since_boot, u32& ticks_this_second, bool query_only)
{
    (void)seconds_since_boot;
    (void)ticks_this_second;
    (void)query_only;
    // TODO_RISCV64();
    return 10;
}

void Timer::enable_interrupt_mode()
{
    set_compare(microseconds_since_boot() + m_interrupt_interval);
    enable_irq();
}

void Timer::set_interrupt_interval_usec(u32 interrupt_interval)
{
    m_interrupt_interval = interrupt_interval;
}

void Timer::clear_interrupt()
{
    // TODO_RISCV64();
}

void Timer::set_compare(u64 compare)
{
    if (SBI::Timer::set_timer(compare).is_error()) {
        MUST(SBI::Legacy::set_timer(compare));
    }
}

}
