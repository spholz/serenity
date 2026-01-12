/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
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
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel::PCI {

class CixSky1PCIeHostController final : public MemoryBackedHostBridge {
public:
    static ErrorOr<NonnullOwnPtr<CixSky1PCIeHostController>> create(DeviceTree::Device const& device, DeviceTree::Device::Resource configuration_space)
    {
        auto domain = TRY(determine_pci_domain_for_devicetree_node(device.node(), device.node_name()));

        if (configuration_space.size < memory_range_per_bus * (domain.end_bus() - domain.start_bus() + 1))
            return ERANGE;

        return adopt_nonnull_own_or_enomem(new (nothrow) CixSky1PCIeHostController(domain, configuration_space.paddr));
    }

protected:
    CixSky1PCIeHostController(Domain const& domain, PhysicalAddress physical_address)
        : MemoryBackedHostBridge(domain, physical_address)
    {
    }
};

static constinit Array const compatibles_array = {
    "cix,sky1-pcie-host"sv,
};

DEVICETREE_DRIVER(CixSky1PCIeHostControllerDriver, compatibles_array);

ErrorOr<void> CixSky1PCIeHostControllerDriver::probe(DeviceTree::Device const& device, StringView) const
{
    if (kernel_command_line().is_pci_disabled())
        return {};

    auto configuration_space = TRY(device.get_resource(2));

    auto bus_range = device.node().get_property("bus-range"sv)->as<Array<BigEndian<u32>, 2>>();
    auto bus_start = bus_range[0];
    if (bus_start != 0x90 && bus_start != 0x30 && bus_start != 0x00)
        return ENOTSUP;

    dbgln("{}: ECAM @ {}", device.node_name(), configuration_space.paddr);

    auto host_controller = TRY(CixSky1PCIeHostController::create(device, configuration_space));

    TRY(configure_devicetree_host_controller(*host_controller, device.node(), device.node_name()));
    Access::the().add_host_controller(move(host_controller));

    return {};
}

}
