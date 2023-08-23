/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Processor.h>
#include <Kernel/Arch/TrapFrame.h>
#include <Kernel/Arch/riscv64/ASM_wrapper.h>
#include <Kernel/Interrupts/InterruptDisabler.h>
#include <Kernel/Sections.h>
#include <Kernel/Security/Random.h>
#include <Kernel/Tasks/Process.h>
#include <Kernel/Tasks/Scheduler.h>

namespace Kernel {

extern "C" void thread_context_first_enter(void);
extern "C" void exit_kernel_thread(void);
extern "C" void context_first_init(Thread* from_thread, Thread* to_thread) __attribute__((used));

Processor* g_current_processor;
READONLY_AFTER_INIT FPUState Processor::s_clean_fpu_state;

// static void store_fpu_state(FPUState* fpu_state)
// {
//     (void)fpu_state;
//     // TODO
// }
//
// static void load_fpu_state(FPUState* fpu_state)
// {
//     (void)fpu_state;
//     // TODO
// }

u32 Processor::smp_wake_n_idle_processors(u32 wake_count)
{
    (void)wake_count;
    // FIXME: Actually wake up other cores when SMP is supported for riscv64.
    return 0;
}

[[noreturn]] void Processor::halt()
{
    // WFI ignores the value of sstatus.SIE, so we can't use disable_interrupts().
    // Instead, disable all interrupts sources by setting SIE to all-zeroes.
    asm volatile("csrw sie, zero");
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

    // FIXME: Figure out if we need to call {pre_,post_,}init_finished once riscv64 supports SMP
    Processor::set_current_in_scheduler(true);

    m_in_critical = 0; // FIXME

    auto& regs = initial_thread.regs();
    // clang-format off
    asm volatile(
        "mv sp, %[new_sp] \n"

        "addi sp, sp, -32 \n"
        "sd %[from_to_thread], 0(sp) \n"
        "sd %[from_to_thread], 8(sp) \n"

        "jr %[new_ip] \n"
        :: [new_sp] "r" (regs.sp()),
        [new_ip] "r" (regs.ip()),
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
        // Store a RegisterState of from_thread on from_thread's stack
        "addi sp, sp, -(34 * 8) \n"

        "sd x1, 0*8(sp) \n"
        // sp
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

        // Store current sp as from_thread's sp.
        "sd sp, %[from_sp] \n"

        // Set from_thread's pc to label "1"
        "la t0, 1f \n"
        "sd t0, %[from_ip] \n"

        // Switch to to_thread's stack
        "ld sp, %[to_sp] \n"

        // Store from_thread, to_thread, to_ip on to_thread's stack
        "addi sp, sp, -(4 * 8) \n"
        "ld a0, %[from_thread] \n"
        "ld a1, %[to_thread] \n"
        "ld t2, %[to_ip] \n"
        "sd a0, 0*8(sp) \n"
        "sd a1, 1*8(sp) \n"
        "sd t2, 2*8(sp) \n"

        // enter_thread_context(from_thread, to_thread)
        "call enter_thread_context \n"

        // Jump to to_ip
        "ld t0, 2*8(sp) \n"
        "jr t0 \n"

        // A thread enters here when they were already scheduled at least once
        "1: \n"
        "addi sp, sp, 4 * 8 \n"

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

        "addi sp, sp, -(4 * 8) \n"
        "ld t0, 0*8(sp) \n"
        "ld t1, 1*8(sp) \n"
        "sd t0, %[from_thread] \n"
        "sd t1, %[to_thread] \n"

        "addi sp, sp, (34 * 8) + (4 * 8) \n"
        :
        [from_ip] "=m"(from_thread->regs().pc),
        [from_sp] "=m"(from_thread->regs().x[1]),
        "=m"(from_thread),
        "=m"(to_thread)

        : [to_ip] "m"(to_thread->regs().pc),
        [to_sp] "m"(to_thread->regs().x[1]),
        [from_thread] "m"(from_thread),
        [to_thread] "m"(to_thread)
        : "memory", "t0", "t1", "t2", "a0", "a1");
    // clang-format on

    // dbgln_if(CONTEXT_SWITCH_DEBUG, "switch_context <-- from {} {} to {} {}", VirtualAddress(from_thread), *from_thread, VirtualAddress(to_thread), *to_thread);
}

void Processor::assume_context(Thread& thread, InterruptsState new_interrupts_state)
{
    (void)thread;
    (void)new_interrupts_state;
    TODO_RISCV64();
}

FlatPtr Processor::init_context(Thread& thread, bool leave_crit)
{
    VERIFY(g_scheduler_lock.is_locked());
    if (leave_crit) {
        // Leave the critical section we set up in Process::exec,
        // but because we still have the scheduler lock we should end up with 1
        VERIFY(in_critical() == 2);
        m_in_critical = 1; // leave it without triggering anything or restoring flags
    }

    u64 kernel_stack_top = thread.kernel_stack_top();

    // Add a random offset between 0-256 (16-byte aligned)
    kernel_stack_top -= round_up_to_power_of_two(get_fast_random<u8>(), 16);

    u64 stack_top = kernel_stack_top;
    dbgln("kernel_stack_top: {:p}", kernel_stack_top);

    auto& thread_regs = thread.regs();
    dbgln("thread_regs.satp: {:p}", thread_regs.satp);

    // Push a RegisterState and TrapFrame onto the stack, which will be popped of the stack and restored into the
    // state of the processor by restore_previous_context.
    stack_top -= sizeof(RegisterState);
    RegisterState& frame = *reinterpret_cast<RegisterState*>(stack_top);
    memcpy(frame.x, thread_regs.x, sizeof(thread_regs.x));

    // We don't overwrite the return address register if it's not 0, since that means this thread's register state was already initialized with
    // an existing return address register value (e.g. it was fork()'ed), so we assume exit_kernel_thread is already saved as previous RA on the
    // stack somewhere.
    if (frame.x[0] == 0x0) {
        // x1 is the return address register for the riscv64 ABI, so this will return to exit_kernel_thread when main thread function returns.
        frame.x[0] = FlatPtr(&exit_kernel_thread);
    }
    frame.pc = thread_regs.pc;
    frame.user_sp = thread_regs.sp();
    frame.sstatus = thread_regs.sstatus;

    // Push a TrapFrame onto the stack
    stack_top -= sizeof(TrapFrame);
    TrapFrame& trap = *reinterpret_cast<TrapFrame*>(stack_top);
    trap.regs = &frame;
    trap.next_trap = nullptr;

    if constexpr (CONTEXT_SWITCH_DEBUG) {
        dbgln("init_context {} ({}) set up to execute at ip={}, sp={}, stack_top={}",
            thread,
            VirtualAddress(&thread),
            VirtualAddress(thread_regs.pc),
            VirtualAddress(thread_regs.sp()),
            VirtualAddress(stack_top));
    }

    // This make sure the thread first executes thread_context_first_enter, which will actually call restore_previous_context
    // which restores the context set up above.
    thread_regs.set_sp(stack_top);
    thread_regs.set_ip(FlatPtr(&thread_context_first_enter));

    return stack_top;
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

NAKED void thread_context_first_enter(void)
{
    asm(
        "ld a0, 0(sp) \n"
        "ld a1, 8(sp) \n"
        "addi sp, sp, 32 \n"
        "call context_first_init \n"
        "tail restore_context \n");
}

void exit_kernel_thread(void)
{
    Thread::current()->exit();
}

extern "C" void context_first_init(Thread* from_thread, Thread* to_thread)
{
    dbgln("last Processor::enable_interrupts(): {}:{} {}", Processor::current().last_interrupt_enable_file, Processor::current().last_interrupt_enable_line, Processor::current().last_interrupt_enable_function);
    VERIFY_INTERRUPTS_DISABLED();
    dbgln_if(CONTEXT_SWITCH_DEBUG, "switch_context <-- from {} {} to {} {} (context_first_init)", VirtualAddress(from_thread), *from_thread, VirtualAddress(to_thread), *to_thread);

    VERIFY(to_thread == Thread::current());

    Scheduler::enter_current(*from_thread);

    auto in_critical = to_thread->saved_critical();
    VERIFY(in_critical > 0);
    Processor::restore_critical(in_critical);

    // Since we got here and don't have Scheduler::context_switch in the
    // call stack (because this is the first time we switched into this
    // context), we need to notify the scheduler so that it can release
    // the scheduler lock. We don't want to enable interrupts at this point
    // as we're still in the middle of a context switch. Doing so could
    // trigger a context switch within a context switch, leading to a crash.
    Scheduler::leave_on_first_switch(InterruptsState::Disabled);

    dbgln("sstatus: {}", bit_cast<RISCV64::Sstatus>(to_thread->regs().sstatus));
}

extern "C" [[gnu::used]] void enter_thread_context(Thread* from_thread, Thread* to_thread);
extern "C" [[gnu::used]] void enter_thread_context(Thread* from_thread, Thread* to_thread)
{
    VERIFY(from_thread == to_thread || from_thread->state() != Thread::State::Running);
    VERIFY(to_thread->state() == Thread::State::Running);

    Processor::set_current_thread(*to_thread);

    // store_fpu_state(&from_thread->fpu_state());

    auto& from_regs = from_thread->regs();
    auto& to_regs = to_thread->regs();
    if (from_regs.satp != to_regs.satp) {
        RISCV64::Asm::set_satp(to_regs.satp);
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
