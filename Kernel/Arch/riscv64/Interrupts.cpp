#include <AK/Error.h>

#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/aarch64/CPU.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel {
GenericInterruptHandler& get_interrupt_handler(u8)
{
    TODO_RISCV64();
}

void register_generic_interrupt_handler(u8, GenericInterruptHandler&)
{
    TODO_RISCV64();
}

void unregister_generic_interrupt_handler(u8, GenericInterruptHandler&)
{
    TODO_RISCV64();
}

// Sets the reserved flag on `number_of_irqs` if it finds unused interrupt handler on
// a contiguous range.
ErrorOr<u8> reserve_interrupt_handlers([[maybe_unused]] u8 number_of_irqs)
{
    TODO_RISCV64();
}

void dump_registers(RegisterState const &regs)
{
    TODO_RISCV64();
}

extern "C" [[noreturn]] [[gnu::aligned(4)]] void trap_handler_nommu()
{
    dbgln_without_mmu("UNHANDLED TRAP (nommu)!"sv);

    for (;;)
        asm volatile("wfi");
}

extern "C" [[noreturn]] void trap_handler();
extern "C" [[noreturn]] [[gnu::aligned(4)]] void trap_handler()
{
    dbgln_without_mmu("UNHANDLED TRAP!"sv);

    uintptr_t scause;
    asm volatile("csrr %0, scause"
                 : "=r"(scause));
    critical_dmesgln("scause: {:#x}", scause);

    uintptr_t sepc;
    asm volatile("csrr %0, sepc"
                 : "=r"(sepc));
    critical_dmesgln("sepc: {:#x}", sepc);

    uintptr_t stval;
    asm volatile("csrr %0, stval"
                 : "=r"(stval));
    critical_dmesgln("stval: {:#x}", stval);

    uintptr_t sstatus;
    asm volatile("csrr %0, sstatus"
                 : "=r"(sstatus));
    critical_dmesgln("sstatus: {:#x}", sstatus);

    uintptr_t satp;
    asm volatile("csrr %0, satp"
                 : "=r"(satp));
    critical_dmesgln("satp: {:#x}", satp);

    Processor::halt();
}

}
