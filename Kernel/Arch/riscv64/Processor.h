/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Function.h>
#include <AK/Types.h>
#include <AK/Vector.h>

#include <Kernel/API/POSIX/errno.h>
#include <Kernel/Arch/DeferredCallPool.h>
#include <Kernel/Arch/ProcessorSpecificDataID.h>
#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Memory/VirtualAddress.h>

struct FPUState {
};

namespace Kernel {

namespace Memory {
class PageDirectory;
};

class Thread;
class Processor;
struct TrapFrame;
enum class InterruptsState;

// FIXME: Remove this once we support SMP in riscv64
extern Processor* g_current_processor;

constexpr size_t MAX_CPU_COUNT = 1;

class Processor {
    void* m_processor_specific_data[static_cast<size_t>(ProcessorSpecificDataID::__Count)];

public:
    Processor() = default;

    void early_initialize([[maybe_unused]] u32 cpu)
    {
        VERIFY(g_current_processor == nullptr);
        g_current_processor = this;
    }

    void initialize([[maybe_unused]] u32 cpu)
    {
        dbgln("FIXME: implement Processor::initialize() for riscv64");
    }

    template<typename T>
    T* get_specific()
    {
        return static_cast<T*>(m_processor_specific_data[static_cast<size_t>(T::processor_specific_data_id())]);
    }

    void set_specific(ProcessorSpecificDataID specific_id, void* ptr)
    {
        m_processor_specific_data[static_cast<size_t>(specific_id)] = ptr;
    }

    void idle_begin() const
    {
        // FIXME: Implement this when SMP for riscv64 is supported.
    }

    void idle_end() const
    {
        // FIXME: Implement this when SMP for riscv64 is supported.
    }

    void wait_for_interrupt() const
    {
        asm("wfi");
    }

    ALWAYS_INLINE static FPUState const& clean_fpu_state()
    {
        return s_clean_fpu_state;
    }

    ALWAYS_INLINE static void set_current_thread([[maybe_unused]] Thread& current_thread)
    {
        current().m_current_thread = &current_thread;
    }

    ALWAYS_INLINE static Thread* idle_thread()
    {
        return current().m_idle_thread;
    }

    ALWAYS_INLINE static Processor& current()
    {
        return *g_current_processor;
    }

    template<IteratorFunction<Processor&> Callback>
    static inline IterationDecision for_each(Callback callback)
    {
        // FIXME: Once we support SMP for riscv64, make sure to call the callback for every processor.
        if (callback(*g_current_processor) == IterationDecision::Break)
            return IterationDecision::Break;
        return IterationDecision::Continue;
    }

    template<VoidFunction<Processor&> Callback>
    static inline IterationDecision for_each(Callback callback)
    {
        // FIXME: Once we support SMP for riscv64, make sure to call the callback for every processor.
        callback(*g_current_processor);
        return IterationDecision::Continue;
    }

    static u32 count()
    {
        return 1;
    }

    ALWAYS_INLINE static bool is_bootstrap_processor()
    {
        return Processor::current_id() == 0;
    }

    void check_invoke_scheduler();
    void invoke_scheduler_async() { m_invoke_scheduler_async = true; }

    ALWAYS_INLINE static bool current_in_scheduler()
    {
        return current().m_in_scheduler;
    }

    ALWAYS_INLINE static void set_current_in_scheduler([[maybe_unused]] bool value)
    {
        current().m_in_scheduler = value;
    }

    ALWAYS_INLINE static void enter_critical()
    {
        auto& current_processor = current();
        current_processor.m_in_critical = current_processor.m_in_critical + 1;
    }

    static void leave_critical();
    static u32 clear_critical();

    ALWAYS_INLINE static void restore_critical([[maybe_unused]] u32 prev_critical)
    {
        current().m_in_critical = prev_critical;
    }

    ALWAYS_INLINE static u32 in_critical()
    {
        return current().m_in_critical;
    }

    ALWAYS_INLINE static void verify_no_spinlocks_held()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static bool is_initialized()
    {
        return g_current_processor != nullptr;
    }

    static void flush_tlb_local([[maybe_unused]] VirtualAddress vaddr, [[maybe_unused]] size_t page_count)
    {
        // TODO: sfence.vma specific pages
        asm volatile("sfence.vma");
    }

    static void flush_tlb(Memory::PageDirectory const*, VirtualAddress, size_t)
    {
        asm volatile("sfence.vma");
    }

    ALWAYS_INLINE u32 id() const
    {
        // NOTE: This variant should only be used when iterating over all
        // Processor instances, or when it's guaranteed that the thread
        // cannot move to another processor in between calling Processor::current
        // and Processor::get_id, or if this fact is not important.
        // All other cases should use Processor::id instead!
        return 0;
    }

    // FIXME: When riscv64 supports multiple cores, return the correct core id here.
    ALWAYS_INLINE static u32 current_id()
    {
        return 0;
    }

    ALWAYS_INLINE void set_idle_thread(Thread& idle_thread)
    {
        m_idle_thread = &idle_thread;
    }

    ALWAYS_INLINE static Thread* current_thread()
    {
        return current().m_current_thread;
    }

    ALWAYS_INLINE bool has_nx() const
    {
        return true;
    }

    ALWAYS_INLINE bool has_pat() const
    {
        return false;
    }

    ALWAYS_INLINE static FlatPtr current_in_irq()
    {
        return current().m_in_irq;
    }

    ALWAYS_INLINE static u64 read_cpu_counter()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static bool are_interrupts_enabled()
    {
        return RISCV64::Sstatus::read().SIE == 1;
    }

    ALWAYS_INLINE static void enable_interrupts(char const* const file = __builtin_FILE(), u32 line = __builtin_LINE(), char const* const function = __builtin_FUNCTION())
    {
        current().last_interrupt_enable_file = file;
        current().last_interrupt_enable_line = line;
        current().last_interrupt_enable_function = function;
        asm volatile("csrsi sstatus, 1 << 1 /* sie */");
    }

    ALWAYS_INLINE static void disable_interrupts()
    {
        asm volatile("csrci sstatus, 1 << 1 /* sie */");
    }

    ALWAYS_INLINE static void pause()
    {
        asm volatile("pause");
    }

    ALWAYS_INLINE static void wait_check()
    {
        Processor::pause();
        // FIXME: Process SMP messages once we support SMP on riscv64; cf. x86_64
    }

    static void deferred_call_queue(Function<void()>)
    {
        TODO_RISCV64();
    }

    u64 time_spent_idle() const
    {
        TODO_RISCV64();
    }

    static u32 smp_wake_n_idle_processors(u32 wake_count);

    [[noreturn]] static void halt();

    [[noreturn]] void initialize_context_switching(Thread& initial_thread);
    NEVER_INLINE void switch_context(Thread*& from_thread, Thread*& to_thread);
    [[noreturn]] static void assume_context(Thread& thread, InterruptsState new_interrupts_state);
    FlatPtr init_context(Thread& thread, bool leave_crit);
    static ErrorOr<Vector<FlatPtr, 32>> capture_stack_trace(Thread& thread, size_t max_frames = 0);

    void enter_trap(TrapFrame& trap, bool raise_irq);
    void exit_trap(TrapFrame& trap);

    static StringView platform_string();

    static void set_thread_specific_data(VirtualAddress thread_specific_data);

    static void flush_entire_tlb_local();

    char const* last_interrupt_enable_file;
    char const* last_interrupt_enable_function;
    u32 last_interrupt_enable_line;

private:
    Processor(Processor const&) = delete;

    void do_leave_critical();

    DeferredCallPool m_deferred_call_pool {};

    Thread* m_current_thread;
    Thread* m_idle_thread;
    u32 m_in_critical { 0 };

    static FPUState s_clean_fpu_state;

    FlatPtr m_in_irq { 0 };
    bool m_in_scheduler { false };
    bool m_invoke_scheduler_async { false };
    bool m_scheduler_initialized { false };
};
}
