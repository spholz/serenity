#include <AK/Function.h>
#include <AK/Types.h>
#include <AK/Vector.h>

#include <Kernel/API/POSIX/errno.h>
#include <Kernel/Arch/ProcessorSpecificDataID.h>
#include <Kernel/Memory/VirtualAddress.h>

struct FPUState {
};

namespace Kernel {

namespace Memory {
class PageDirectory;
};

class Thread;
class Processor;
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
        TODO_RISCV64();
    }

    void idle_end() const
    {
        TODO_RISCV64();
    }

    void wait_for_interrupt() const
    {
        asm("wfi");
    }

    ALWAYS_INLINE static FPUState const& clean_fpu_state()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void set_current_thread([[maybe_unused]] Thread& current_thread)
    {
        TODO_RISCV64();
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
        TODO_RISCV64();
    }

    ALWAYS_INLINE static bool is_bootstrap_processor()
    {
        return Processor::current_id() == 0;
    }

    void invoke_scheduler_async() { TODO_RISCV64(); }

    ALWAYS_INLINE static bool current_in_scheduler()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void set_current_in_scheduler([[maybe_unused]] bool value)
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void enter_critical()
    {
        // dbgln("FIXME: implement Processor::enter_critical() for riscv64");
    }

    static void leave_critical()
    {
        // dbgln("FIXME: implement Processor::leave_critical() for riscv64");
    }

    static u32 clear_critical()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void restore_critical([[maybe_unused]] u32 prev_critical)
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static u32 in_critical()
    {
        TODO_RISCV64();
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
        TODO_RISCV64();
    }

    ALWAYS_INLINE static u64 read_cpu_counter()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static bool are_interrupts_enabled()
    {
        return false;
    }

    ALWAYS_INLINE static void enable_interrupts()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void disable_interrupts()
    {
        asm volatile("csrci sstatus, 1 << 1 /* sie */");
    }

    ALWAYS_INLINE static void pause()
    {
        TODO_RISCV64();
    }

    ALWAYS_INLINE static void wait_check()
    {
        TODO_RISCV64();
    }

    static void deferred_call_queue(Function<void()>)
    {
        TODO_RISCV64();
    }

    u64 time_spent_idle() const
    {
        TODO_RISCV64();
    }

    static u32 smp_wake_n_idle_processors(u32)
    {
        TODO_RISCV64();
    }

    [[noreturn]] static void halt()
    {
        for (;;)
            asm volatile("wfi");
    }

    [[noreturn]] void initialize_context_switching(Thread&)
    {
        TODO_RISCV64();
    }

    NEVER_INLINE void switch_context(Thread*&, Thread*&)
    {
        TODO_RISCV64();
    }

    [[noreturn]] static void assume_context(Thread&, InterruptsState)
    {
        TODO_RISCV64();
    }

    FlatPtr init_context(Thread&, bool)
    {
        TODO_RISCV64();
    }

    static ErrorOr<Vector<FlatPtr, 32>> capture_stack_trace([[maybe_unused]] Thread& thread, [[maybe_unused]] size_t max_frames = 0)
    {
        TODO_RISCV64();
    }

    static StringView platform_string()
    {
        TODO_RISCV64();
    }

    static void set_thread_specific_data(VirtualAddress)
    {
        TODO_RISCV64();
    }

private:
    Thread* m_current_thread;
    Thread* m_idle_thread;
};
}
