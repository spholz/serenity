/*
 * Copyright (c) 2024, Idan Horowitz <idan.horowitz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/USB/xHCI/xHCIInterrupter.h>

namespace Kernel::USB {

ErrorOr<NonnullOwnPtr<xHCIPCIInterrupter>> xHCIPCIInterrupter::create(xHCIController& controller, u16 interrupter_id)
{
    auto irq = TRY(controller.m_pci_device->allocate_irq(0));
    return TRY(adopt_nonnull_own_or_enomem(new (nothrow) xHCIPCIInterrupter(controller, interrupter_id, irq)));
}

xHCIPCIInterrupter::xHCIPCIInterrupter(xHCIController& controller, u16 interrupter_id, u16 irq)
    : PCI::IRQHandler(*controller.m_pci_device, irq)
    , m_controller(controller)
    , m_interrupter_id(interrupter_id)
{
    enable_irq();
}

bool xHCIPCIInterrupter::handle_irq()
{
    m_controller.handle_interrupt({}, m_interrupter_id);
    return true;
}

}
