/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/riscv64/IRQController.h>
#include <Kernel/Arch/riscv64/InterruptManagement.h>
#include <Kernel/Arch/riscv64/PLIC.h>
#include <Kernel/Interrupts/SharedIRQHandler.h>

namespace Kernel {

static InterruptManagement* s_interrupt_management;

bool InterruptManagement::initialized()
{
    return s_interrupt_management != nullptr;
}

InterruptManagement& InterruptManagement::the()
{
    VERIFY(InterruptManagement::initialized());
    return *s_interrupt_management;
}

void InterruptManagement::initialize()
{
    VERIFY(!InterruptManagement::initialized());
    s_interrupt_management = new InterruptManagement;

    the().find_controllers();
}

void InterruptManagement::find_controllers()
{
    // TODO: Once device tree support is in place, find interrupt controllers using that.
    // m_interrupt_controllers.append(RISCV64::PLIC::try_to_initialize(PhysicalAddress { 0xc000000 }).release_value_but_fixme_should_propagate_errors());
}

u8 InterruptManagement::acquire_mapped_interrupt_number(u8 original_irq)
{
    return original_irq;
}

Vector<NonnullLockRefPtr<IRQController>> const& InterruptManagement::controllers()
{
    return m_interrupt_controllers;
}

NonnullLockRefPtr<IRQController> InterruptManagement::get_responsible_irq_controller(size_t)
{
    // TODO: Support more interrupt controllers
    // VERIFY(m_interrupt_controllers.size() == 1);
    // return m_interrupt_controllers[0];
    TODO_RISCV64();
}

void InterruptManagement::enumerate_interrupt_handlers(Function<void(GenericInterruptHandler&)> callback)
{
    for (size_t i = 0; i < 64; i++) {
        auto& handler = get_interrupt_handler(i);
        if (handler.type() == HandlerType::SharedIRQHandler) {
            static_cast<SharedIRQHandler&>(handler).enumerate_handlers(callback);
            continue;
        }
        if (handler.type() != HandlerType::UnhandledInterruptHandler)
            callback(handler);
    }
}

}
