/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/USB/USBManagement.h>
#include <Kernel/Bus/USB/xHCI/DeviceTreexHCIController.h>
#include <Kernel/Bus/USB/xHCI/xHCIInterrupter.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>

namespace Kernel::USB::xHCI {

ErrorOr<NonnullLockRefPtr<DeviceTreexHCIController>> DeviceTreexHCIController::try_to_initialize(DeviceTree::Device::Resource registers_resource, StringView node_name, size_t interrupt_number)
{
    auto registers_mapping = TRY(Memory::map_typed<u8>(registers_resource.paddr, registers_resource.size, Memory::Region::Access::ReadWrite));

    auto controller = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) DeviceTreexHCIController(move(registers_mapping), node_name, interrupt_number)));
    TRY(controller->initialize());
    return controller;
}

UNMAP_AFTER_INIT DeviceTreexHCIController::DeviceTreexHCIController(Memory::TypedMapping<u8> registers_mapping, StringView node_name, size_t interrupt_number)
    : xHCIController(move(registers_mapping))
    , m_node_name(node_name)
    , m_interrupt_number(interrupt_number)
{
}

ErrorOr<OwnPtr<GenericInterruptHandler>> DeviceTreexHCIController::create_interrupter(u16 interrupter_id)
{
    (void)interrupter_id;
    return nullptr;
    // return TRY(xHCIDeviceTreeInterrupter::create(*this, m_interrupt_number, interrupter_id));
}

static constinit Array const compatibles_array = {
    "generic-xhci"sv,
    "cdns,usbssp"sv,
};

DEVICETREE_DRIVER(DeviceTreexHCIControllerDriver, compatibles_array);

// https://www.kernel.org/doc/Documentation/devicetree/bindings/usb/generic-xhci.yaml
ErrorOr<void> DeviceTreexHCIControllerDriver::probe(DeviceTree::Device const& device, StringView compatible) const
{
    size_t registers_resource_index = 0;
    if (compatible == "cdns,usbssp"sv)
        registers_resource_index = 2;

    auto registers_resource = TRY(device.get_resource(registers_resource_index));
    auto interrupt_number = 0;

    auto controller = TRY(DeviceTreexHCIController::try_to_initialize(registers_resource, device.node_name(), interrupt_number));
    USB::USBManagement::the().add_controller(controller);

    return {};
}

}
