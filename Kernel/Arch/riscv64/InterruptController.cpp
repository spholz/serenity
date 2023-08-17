/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/BuiltinWrappers.h>
#include <Kernel/Arch/riscv64/InterruptController.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel::RISCV64 {

InterruptController::InterruptController()
{
}

void InterruptController::enable(GenericInterruptHandler const& handler)
{
    u8 interrupt_number = handler.interrupt_number();
    VERIFY(interrupt_number < 64);
    asm volatile("csrs sie, %0" ::"r"(1 << interrupt_number));
}

void InterruptController::disable(GenericInterruptHandler const& handler)
{
    u8 interrupt_number = handler.interrupt_number();
    VERIFY(interrupt_number < 64);
    asm volatile("csrc sie, %0" ::"r"(1 << interrupt_number));
}

void InterruptController::eoi(GenericInterruptHandler const&) const
{
}

u64 InterruptController::pending_interrupts() const
{

    uintptr_t sip;
    asm volatile("csrr %0, sip"
                 : "=r"(sip));

    return popcount(sip);
}

}
