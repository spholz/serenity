/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Error.h>

#include <Kernel/Arch/CPU.h>
#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/PageFault.h>
#include <Kernel/Arch/riscv64/ASM_wrapper.h>
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
    return number_of_irqs;
}

void dump_registers(RegisterState const& regs);
void dump_registers(RegisterState const& regs)
{
    auto scause = RiscV64::Asm::get_scause();
    auto stval = RiscV64::Asm::get_stval();
    auto satp = RiscV64::Asm::get_satp();

    dbgln("scause:  {} ({:p})", RiscV64::scause_to_string(scause), scause);
    dbgln("sepc:    {:p}", regs.sepc);
    dbgln("stval:   {:p}", stval);
    dbgln("sstatus: {:p}", regs.sstatus);
    dbgln("satp:    {:p}", satp);

    dbgln(" ra( x1)={:p}  sp( x2)={:p}  gp( x3)={:p}  tp( x4)={:p}  t0( x5)={:p}", regs.x[0], regs.x[1], regs.x[2], regs.x[3], regs.x[4]);
    dbgln(" t1( x6)={:p}  t2( x7)={:p}  s0( x8)={:p}  s1( x9)={:p}  a0(x10)={:p}", regs.x[5], regs.x[6], regs.x[7], regs.x[8], regs.x[9]);
    dbgln(" a1(x11)={:p}  a2(x12)={:p}  a3(x13)={:p}  a4(x14)={:p}  a5(x15)={:p}", regs.x[10], regs.x[11], regs.x[12], regs.x[13], regs.x[14]);
    dbgln(" a6(x16)={:p}  a7(x17)={:p}  s2(x18)={:p}  s3(x19)={:p}  s4(x20)={:p}", regs.x[15], regs.x[16], regs.x[17], regs.x[18], regs.x[19]);
    dbgln(" s5(x21)={:p}  s6(x22)={:p}  s7(x23)={:p}  s8(x24)={:p}  s9(x25)={:p}", regs.x[20], regs.x[21], regs.x[22], regs.x[23], regs.x[24]);
    dbgln("s10(x26)={:p} s11(x27)={:p}  t3(x28)={:p}  t4(x29)={:p}  t5(x30)={:p}", regs.x[25], regs.x[26], regs.x[27], regs.x[28], regs.x[29]);
    dbgln(" t6(x31)={:p}", regs.x[30]);
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
    auto scause = RiscV64::Asm::get_scause();

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

        if (RiscV64::scause_is_page_fault(scause)) {
            auto stval = RiscV64::Asm::get_stval();
            PageFault fault { VirtualAddress(stval) };

            if (scause == 12)
                fault.set_instruction_fetch(true);
            else if (scause == 13)
                fault.set_access(PageFault::Access::Read);
            else if (scause == 15)
                fault.set_access(PageFault::Access::Write);

            fault.set_type(PageFault::Type::ProtectionViolation);

            fault.handle(*trap_frame.regs);
        } else {
            handle_crash(*trap_frame.regs, "Unexpected exception", SIGSEGV, false);
        }

        Processor::current().exit_trap(trap_frame);
    }
}
}
