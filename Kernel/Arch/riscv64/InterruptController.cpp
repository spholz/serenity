/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/riscv64/InterruptController.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel::RiscV {

InterruptController::InterruptController()
{
}

void InterruptController::enable(GenericInterruptHandler const& handler)
{
    u8 interrupt_number = handler.interrupt_number();
    VERIFY(interrupt_number < 64);
}

void InterruptController::disable(GenericInterruptHandler const& handler)
{
    u8 interrupt_number = handler.interrupt_number();
    VERIFY(interrupt_number < 64);
}

void InterruptController::eoi(GenericInterruptHandler const&) const
{
}

u64 InterruptController::pending_interrupts() const
{
    return 1;
}

}
