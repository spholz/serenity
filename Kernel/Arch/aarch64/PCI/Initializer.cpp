/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <AK/SetOnce.h>
#include <Kernel/Arch/aarch64/PCI/Controller/SMCCCHostBridge.h>
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Bus/PCI/Initializer.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Bus/PCI/BusDirectory.h>

// Initializer.cpp
namespace Kernel::PCI {

SetOnce g_pci_access_io_probe_failed;
SetOnce g_pci_access_is_disabled_from_commandline;

void initialize()
{
    if (kernel_command_line().is_pci_disabled()) {
        g_pci_access_is_disabled_from_commandline.set();
        return;
    }

    if (g_pci_access_is_disabled_from_commandline.was_set())
        return;

    VERIFY(!Access::is_initialized());
    auto* access = new Access();

    for (size_t i = 2; i < 3; i++) {
        auto host_bridge = SMCCCHostBridge::must_create(i);
        access->add_host_controller(move(host_bridge));
    }

    PCIConfiguration config {
        0x1f'0000'0000,
        0x1f'0000'0000 + 0xffff'fffc,
        0x1c'0000'0000,
        0x1c'0000'0000 + 0x03'0000'0000,
        {}, // XXX: Interrupts
        {},
    };

    dbgln("configure_pci_space");
    access->configure_pci_space(config);
    dbgln("rescan_hardware");
    access->rescan_hardware();

    PCIBusSysFSDirectory::initialize();

    MUST(PCI::enumerate([&](DeviceIdentifier const& device_identifier) {
        SpinlockLocker locker(device_identifier.operation_lock());
        dmesgln("{} {}", device_identifier.address(), device_identifier.hardware_id());
    }));
}

}
