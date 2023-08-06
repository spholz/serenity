#include <AK/Error.h>

#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/CPU.h>
#include <Kernel/Arch/riscv64/InterruptManagement.h>
#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Arch/riscv64/TrapFrame.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>
#include <Kernel/Interrupts/SharedIRQHandler.h>
#include <Kernel/Interrupts/UnhandledInterruptHandler.h>
#include <Kernel/Library/Panic.h>
#include <Kernel/Library/StdLib.h>

namespace Kernel {

static Array<GenericInterruptHandler*, 64> s_interrupt_handlers;

GenericInterruptHandler& get_interrupt_handler(u8)
{
    TODO_RISCV64();
}

void register_generic_interrupt_handler(u8 interrupt_number, GenericInterruptHandler& handler)
{
    auto*& handler_slot = s_interrupt_handlers[interrupt_number];
    if (handler_slot == nullptr) {
        handler_slot = &handler;
        return;
    }
    if (handler_slot->type() == HandlerType::UnhandledInterruptHandler) {
        auto* unhandled_handler = static_cast<UnhandledInterruptHandler*>(handler_slot);
        unhandled_handler->unregister_interrupt_handler();
        delete unhandled_handler;
        handler_slot = &handler;
        return;
    }
    if (handler_slot->is_shared_handler()) {
        VERIFY(handler_slot->type() == HandlerType::SharedIRQHandler);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(handler);
        return;
    }
    if (!handler_slot->is_shared_handler()) {
        if (handler_slot->type() == HandlerType::SpuriousInterruptHandler) {
            // FIXME: Add support for spurious interrupts on aarch64
            TODO_AARCH64();
        }
        VERIFY(handler_slot->type() == HandlerType::IRQHandler);
        auto& previous_handler = *handler_slot;
        handler_slot = nullptr;
        SharedIRQHandler::initialize(interrupt_number);
        VERIFY(handler_slot);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(previous_handler);
        static_cast<SharedIRQHandler*>(handler_slot)->register_handler(handler);
        return;
    }
    VERIFY_NOT_REACHED();
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

void dump_registers(RegisterState const& regs);
void dump_registers(RegisterState const& regs)
{
    uintptr_t scause;
    asm volatile("csrr %0, scause"
                 : "=r"(scause));
    dbgln("scause: {} ({:#x})", RiscV64::scause_to_string(scause), scause);

    dbgln("sepc: {:#x}", regs.sepc);

    uintptr_t stval;
    asm volatile("csrr %0, stval"
                 : "=r"(stval));
    dbgln("stval: {:#x}", stval);

    dbgln("sstatus: {:#x}", regs.sstatus);

    uintptr_t satp;
    asm volatile("csrr %0, satp"
                 : "=r"(satp));
    dbgln("satp: {:#x}", satp);

    dbgln(" x1={:p}  x2={:p}  x3={:p}  x4={:p}  x5={:p}", regs.x[0], regs.x[1], regs.x[2], regs.x[3], regs.x[4]);
    dbgln(" x6={:p}  x7={:p}  x8={:p}  x9={:p} x10={:p}", regs.x[5], regs.x[6], regs.x[7], regs.x[8], regs.x[9]);
    dbgln("x11={:p} x12={:p} x13={:p} x14={:p} x15={:p}", regs.x[10], regs.x[11], regs.x[12], regs.x[13], regs.x[14]);
    dbgln("x16={:p} x17={:p} x18={:p} x19={:p} x20={:p}", regs.x[15], regs.x[16], regs.x[17], regs.x[18], regs.x[19]);
    dbgln("x21={:p} x22={:p} x23={:p} x24={:p} x25={:p}", regs.x[20], regs.x[21], regs.x[22], regs.x[23], regs.x[24]);
    dbgln("x26={:p} x27={:p} x28={:p} x29={:p} x30={:p}", regs.x[25], regs.x[26], regs.x[27], regs.x[28], regs.x[29]);
    dbgln("x31={:p}", regs.x[30]);
}

extern "C" [[noreturn]] [[gnu::aligned(4)]] void trap_handler_nommu();
extern "C" [[noreturn]] [[gnu::aligned(4)]] void trap_handler_nommu()
{
    dbgln_without_mmu("UNHANDLED TRAP (nommu)!"sv);

    for (;;)
        asm volatile("wfi");
}

extern "C" void trap_handler(TrapFrame& trap_frame);
extern "C" void trap_handler(TrapFrame& trap_frame)
{

    uintptr_t scause;
    asm volatile("csrr %0, scause"
                 : "=r"(scause));

    if ((scause >> 63) == 1) {
        // Interrupt
        Processor::current().enter_trap(trap_frame, true);

        auto* handler = s_interrupt_handlers[5];
        VERIFY(handler);
        handler->increment_call_count();
        handler->handle_interrupt(*trap_frame.regs);
        handler->eoi();

        // for (auto& interrupt_controller : InterruptManagement::the().controllers()) {
        //     auto pending_interrupts = interrupt_controller->pending_interrupts();

        //     // TODO: Add these interrupts as a source of entropy for randomness.
        //     u8 irq = 0;
        //     while (pending_interrupts) {
        //         if ((pending_interrupts & 0b1) != 0b1) {
        //             irq += 1;
        //             pending_interrupts >>= 1;
        //             continue;
        //         }

        //         auto* handler = s_interrupt_handlers[irq];
        //         VERIFY(handler);
        //         handler->increment_call_count();
        //         handler->handle_interrupt(*trap_frame.regs);
        //         handler->eoi();

        //         irq += 1;
        //         pending_interrupts >>= 1;
        //     }
        // }

        Processor::current().exit_trap(trap_frame);
    } else {
        // Exception
        Processor::current().enter_trap(trap_frame, false);
        Processor::enable_interrupts();

        dump_registers(*trap_frame.regs);
        handle_crash(*trap_frame.regs, "Unexpected exception", SIGSEGV, false);

        Processor::disable_interrupts();
        Processor::current().exit_trap(trap_frame);
    }

}
}
