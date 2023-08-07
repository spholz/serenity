#include <Kernel/Arch/Processor.h>
#include <Kernel/Arch/TrapFrame.h>
#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Interrupts/InterruptDisabler.h>
#include <Kernel/Sections.h>
#include <Kernel/Tasks/Process.h>
#include <Kernel/Tasks/Scheduler.h>
#include <Kernel/Tasks/Thread.h>
#include <Kernel/Time/TimeManagement.h>

namespace Kernel {

Processor* g_current_processor;
READONLY_AFTER_INIT FPUState Processor::s_clean_fpu_state;

static void store_fpu_state(FPUState* fpu_state)
{
    // TODO
}

static void load_fpu_state(FPUState* fpu_state)
{
    // TODO
}

u32 Processor::smp_wake_n_idle_processors(u32 wake_count)
{
    (void)wake_count;
    // FIXME: Actually wake up other cores when SMP is supported for aarch64.
    return 0;
}

[[noreturn]] void Processor::halt()
{
    // disable_interrupts();
    for (;;)
        asm volatile("wfi");
}

void Processor::flush_entire_tlb_local()
{
    asm volatile("sfence.vma");
}

void Processor::leave_critical()
{
    InterruptDisabler disabler;
    current().do_leave_critical();
}

void Processor::do_leave_critical()
{
    VERIFY(m_in_critical > 0);
    if (m_in_critical == 1) {
        if (m_in_irq == 0) {
            m_deferred_call_pool.execute_pending();
            VERIFY(m_in_critical == 1);
        }
        m_in_critical = 0;
        if (m_in_irq == 0)
            check_invoke_scheduler();
    } else {
        m_in_critical = m_in_critical - 1;
    }
}

u32 Processor::clear_critical()
{
    InterruptDisabler disabler;
    auto prev_critical = in_critical();
    auto& proc = current();
    proc.m_in_critical = 0;
    if (proc.m_in_irq == 0)
        proc.check_invoke_scheduler();
    return prev_critical;
}

void Processor::initialize_context_switching(Thread& initial_thread)
{
    VERIFY(initial_thread.process().is_kernel_process());

    m_scheduler_initialized = true;

    // FIXME: Figure out if we need to call {pre_,post_,}init_finished once aarch64 supports SMP
    Processor::set_current_in_scheduler(true);

    m_in_critical = 0; // FIXME

    auto& regs = initial_thread.regs();
    // clang-format off
    asm volatile(
        "mv sp, %[new_sp] \n"

        "li t0, 1 << 5 /* STIE */\n"
        "csrw sie, t0\n"
        "csrsi sstatus, 1 << 1 /* SIE */\n"

        "addi sp, sp, -32 \n"
        "sd %[from_to_thread], 0(sp) \n"
        "sd %[from_to_thread], 8(sp) \n"

        "jr %[new_ip] \n"
        :: [new_sp] "r" (regs.sp),
        [new_ip] "r" (regs.sepc),
        [from_to_thread] "r" (&initial_thread)
        : "t0"
    );
    // clang-format on

    VERIFY_NOT_REACHED();
}

void Processor::switch_context(Thread*& from_thread, Thread*& to_thread)
{
    VERIFY(!m_in_irq);
    VERIFY(m_in_critical == 1);

    dbgln_if(CONTEXT_SWITCH_DEBUG, "switch_context --> switching out of: {} {}", VirtualAddress(from_thread), *from_thread);

    // m_in_critical is restored in enter_thread_context
    from_thread->save_critical(m_in_critical);

    // clang-format off
    asm volatile(
        "addi sp, sp, -256 \n"
        "sd x1, 0*8(sp) \n"
        "sd x2, 1*8(sp) \n"
        "sd x3, 2*8(sp) \n"
        "sd x4, 3*8(sp) \n"
        "sd x5, 4*8(sp) \n"
        "sd x6, 5*8(sp) \n"
        "sd x7, 6*8(sp) \n"
        "sd x8, 7*8(sp) \n"
        "sd x9, 8*8(sp) \n"
        "sd x10, 9*8(sp) \n"
        "sd x11, 10*8(sp) \n"
        "sd x12, 11*8(sp) \n"
        "sd x13, 12*8(sp) \n"
        "sd x14, 13*8(sp) \n"
        "sd x15, 14*8(sp) \n"
        "sd x16, 15*8(sp) \n"
        "sd x17, 16*8(sp) \n"
        "sd x18, 17*8(sp) \n"
        "sd x19, 18*8(sp) \n"
        "sd x20, 19*8(sp) \n"
        "sd x21, 20*8(sp) \n"
        "sd x22, 21*8(sp) \n"
        "sd x23, 22*8(sp) \n"
        "sd x24, 23*8(sp) \n"
        "sd x25, 24*8(sp) \n"
        "sd x26, 25*8(sp) \n"
        "sd x27, 26*8(sp) \n"
        "sd x28, 27*8(sp) \n"
        "sd x29, 28*8(sp) \n"
        "sd x30, 29*8(sp) \n"
        "sd x31, 30*8(sp) \n"
        "mv t0, sp \n"
        "sd t0, %[from_sp] \n"
        "la t0, 1f \n"
        "sd t0, %[from_ip] \n"

        "ld t0, %[to_sp] \n"
        "mv sp, t0 \n"

        "addi sp, sp, -32 \n"
        "ld a0, %[from_thread] \n"
        "ld a1, %[to_thread] \n"
        "ld t2, %[to_ip] \n"
        "sd a0, 0*8(sp) \n"
        "sd a1, 1*8(sp) \n"
        "sd t2, 2*8(sp) \n"

        "call enter_thread_context \n"
        "ld t0, 2*8(sp) \n"
        "jr t0 \n"

        "1: \n"
        "addi sp, sp, 32 \n"

        "ld x1, 0*8(sp) \n"
        // sp
        "ld x3, 2*8(sp) \n"
        "ld x4, 3*8(sp) \n"
        "ld x5, 4*8(sp) \n"
        "ld x6, 5*8(sp) \n"
        "ld x7, 6*8(sp) \n"
        "ld x8, 7*8(sp) \n"
        "ld x9, 8*8(sp) \n"
        "ld x10, 9*8(sp) \n"
        "ld x11, 10*8(sp) \n"
        "ld x12, 11*8(sp) \n"
        "ld x13, 12*8(sp) \n"
        "ld x14, 13*8(sp) \n"
        "ld x15, 14*8(sp) \n"
        "ld x16, 15*8(sp) \n"
        "ld x17, 16*8(sp) \n"
        "ld x18, 17*8(sp) \n"
        "ld x19, 18*8(sp) \n"
        "ld x20, 19*8(sp) \n"
        "ld x21, 20*8(sp) \n"
        "ld x22, 21*8(sp) \n"
        "ld x23, 22*8(sp) \n"
        "ld x24, 23*8(sp) \n"
        "ld x25, 24*8(sp) \n"
        "ld x26, 25*8(sp) \n"
        "ld x27, 26*8(sp) \n"
        "ld x28, 27*8(sp) \n"
        "ld x29, 28*8(sp) \n"
        "ld x30, 29*8(sp) \n"
        "ld x31, 30*8(sp) \n"

        // Restore sp last
        "ld x2, 1*8(sp) \n"

        "addi sp, sp, -32 \n"
        "ld t0, 0*8(sp) \n"
        "ld t1, 1*8(sp) \n"
        "sd t0, %[from_thread] \n"
        "sd t1, %[to_thread] \n"

        "addi sp, sp, 288 \n"
        :
        [from_ip] "=m"(from_thread->regs().sepc),
        [from_sp] "=m"(from_thread->regs().sp),
        "=m"(from_thread),
        "=m"(to_thread)

        : [to_ip] "m"(to_thread->regs().sepc),
        [to_sp] "m"(to_thread->regs().sp),
        [from_thread] "m"(from_thread),
        [to_thread] "m"(to_thread)
        : "memory", "t0", "t1", "t2", "a0", "a1");
    // clang-format on

    dbgln_if(CONTEXT_SWITCH_DEBUG, "switch_context <-- from {} {} to {} {}", VirtualAddress(from_thread), *from_thread, VirtualAddress(to_thread), *to_thread);
}

void Processor::assume_context(Thread& thread, InterruptsState new_interrupts_state)
{
    TODO_RISCV64();
}

FlatPtr Processor::init_context(Thread& thread, bool leave_crit)
{
    return 0;
}

void Processor::enter_trap(TrapFrame& trap, bool raise_irq)
{
    VERIFY_INTERRUPTS_DISABLED();
    VERIFY(&Processor::current() == this);
    // FIXME: Figure out if we need prev_irq_level, see duplicated code in Kernel/Arch/x86/common/Processor.cpp
    if (raise_irq)
        m_in_irq++;
    auto* current_thread = Processor::current_thread();
    if (current_thread) {
        auto& current_trap = current_thread->current_trap();
        trap.next_trap = current_trap;
        current_trap = &trap;
        auto new_previous_mode = trap.regs->previous_mode();
        if (current_thread->set_previous_mode(new_previous_mode)) {
            current_thread->update_time_scheduled(TimeManagement::scheduler_current_time(), new_previous_mode == ExecutionMode::Kernel, false);
        }
    } else {
        trap.next_trap = nullptr;
    }
}

void Processor::exit_trap(TrapFrame& trap)
{
    VERIFY_INTERRUPTS_DISABLED();
    VERIFY(&Processor::current() == this);

    // Temporarily enter a critical section. This is to prevent critical
    // sections entered and left within e.g. smp_process_pending_messages
    // to trigger a context switch while we're executing this function
    // See the comment at the end of the function why we don't use
    // ScopedCritical here.
    m_in_critical = m_in_critical + 1;

    // FIXME: Figure out if we need prev_irq_level, see duplicated code in Kernel/Arch/x86/common/Processor.cpp
    m_in_irq = 0;

    // Process the deferred call queue. Among other things, this ensures
    // that any pending thread unblocks happen before we enter the scheduler.
    m_deferred_call_pool.execute_pending();

    auto* current_thread = Processor::current_thread();
    if (current_thread) {
        auto& current_trap = current_thread->current_trap();
        current_trap = trap.next_trap;
        ExecutionMode new_previous_mode;
        if (current_trap) {
            VERIFY(current_trap->regs);
            new_previous_mode = current_trap->regs->previous_mode();
        } else {
            // If we don't have a higher level trap then we're back in user mode.
            // Which means that the previous mode prior to being back in user mode was kernel mode
            new_previous_mode = ExecutionMode::Kernel;
        }

        if (current_thread->set_previous_mode(new_previous_mode))
            current_thread->update_time_scheduled(TimeManagement::scheduler_current_time(), true, false);
    }

    VERIFY_INTERRUPTS_DISABLED();

    // Leave the critical section without actually enabling interrupts.
    // We don't want context switches to happen until we're explicitly
    // triggering a switch in check_invoke_scheduler.
    m_in_critical = m_in_critical - 1;
    if (!m_in_irq && !m_in_critical)
        check_invoke_scheduler();
}

ErrorOr<Vector<FlatPtr, 32>> Processor::capture_stack_trace(Thread& thread, size_t max_frames)
{
    (void)thread;
    (void)max_frames;
    dbgln("FIXME: Implement Processor::capture_stack_trace() for riscv64");
    return Vector<FlatPtr, 32> {};
}

void Processor::check_invoke_scheduler()
{
    VERIFY_INTERRUPTS_DISABLED();
    VERIFY(!m_in_irq);
    VERIFY(!m_in_critical);
    VERIFY(&Processor::current() == this);
    if (m_invoke_scheduler_async && m_scheduler_initialized) {
        m_invoke_scheduler_async = false;
        Scheduler::invoke_async();
    }
}

extern "C" void enter_thread_context(Thread* from_thread, Thread* to_thread)
{
    VERIFY(from_thread == to_thread || from_thread->state() != Thread::State::Running);
    VERIFY(to_thread->state() == Thread::State::Running);

    Processor::set_current_thread(*to_thread);

    // store_fpu_state(&from_thread->fpu_state());

    auto& from_regs = from_thread->regs();
    auto& to_regs = to_thread->regs();
    if (from_regs.satp != to_regs.satp) {
        // Aarch64::Asm::set_ttbr0_el1(to_regs.ttbr0_el1);
        Processor::flush_entire_tlb_local();
    }

    to_thread->set_cpu(Processor::current().id());

    Processor::set_thread_specific_data(to_thread->thread_specific_data());

    auto in_critical = to_thread->saved_critical();
    VERIFY(in_critical > 0);
    Processor::restore_critical(in_critical);

    // load_fpu_state(&to_thread->fpu_state());
}

StringView Processor::platform_string()
{
    return "riscv64"sv;
}

void Processor::set_thread_specific_data(VirtualAddress thread_specific_data)
{
    register uintptr_t tp asm("tp") = thread_specific_data.get();
    (void)tp;
}

}
