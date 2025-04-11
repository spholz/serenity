/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Bus/PCI/Controller/MemoryBackedHostBridge.h>
#include <Kernel/Bus/PCI/DeviceTreeHelpers.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>

namespace Kernel::PCI {

// This driver requires the host controller to be already initialized by the firmware.

class JH7110HostController : public MemoryBackedHostBridge {
public:
    static ErrorOr<NonnullOwnPtr<JH7110HostController>> create(DeviceTree::Device const& device)
    {
        auto domain = TRY(determine_pci_domain_for_devicetree_node(device.node(), device.node_name()));
        auto configuration_space = TRY(device.get_resource(1));

        if (configuration_space.size < memory_range_per_bus * (domain.end_bus() - domain.start_bus() + 1))
            return ERANGE;

        return adopt_nonnull_own_or_enomem(new (nothrow) JH7110HostController(domain, configuration_space.paddr));
    }

protected:
    JH7110HostController(Domain const& domain, PhysicalAddress physical_address)
        : MemoryBackedHostBridge(domain, physical_address)
    {
    }

    static bool quirk_is_pci_address_invalid(BusNumber bus, DeviceNumber device, FunctionNumber)
    {
        // Bus 1 (the bus behind the PCI-to-PCI bridge at device 0 of the host bridge bus)
        // seems to be buggy and reports the same device at all device IDs.
        return bus == 1 && device != 0;
    }

    virtual void write8_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u8 value) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return;
        MemoryBackedHostBridge::write8_field_locked(bus, device, function, field, value);
    }

    virtual void write16_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u16 value) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return;
        MemoryBackedHostBridge::write16_field_locked(bus, device, function, field, value);
    }

    virtual void write32_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u32 value) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return;
        MemoryBackedHostBridge::write32_field_locked(bus, device, function, field, value);
    }

    virtual u8 read8_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return 0xff;
        return MemoryBackedHostBridge::read8_field_locked(bus, device, function, field);
    }

    virtual u16 read16_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return 0xffff;
        return MemoryBackedHostBridge::read16_field_locked(bus, device, function, field);
    }

    virtual u32 read32_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field) override
    {
        if (quirk_is_pci_address_invalid(bus, device, function))
            return 0xffff'ffff;
        return MemoryBackedHostBridge::read32_field_locked(bus, device, function, field);
    }
};

static constinit Array const compatibles_array = {
    "starfive,jh7110-pcie"sv,
};

DEVICETREE_DRIVER(JH7110PCIeHostControllerDriver, compatibles_array);

// https://www.kernel.org/doc/Documentation/devicetree/bindings/pci/starfive,jh7110-pcie.yaml
ErrorOr<void> JH7110PCIeHostControllerDriver::probe(DeviceTree::Device const& device, StringView) const
{
    if (kernel_command_line().is_pci_disabled())
        return {};

    auto host_controller = TRY(JH7110HostController::create(device));

    TRY(configure_devicetree_host_controller(*host_controller, device.node(), device.node_name()));
    Access::the().add_host_controller(move(host_controller));

    return {};
}

}
