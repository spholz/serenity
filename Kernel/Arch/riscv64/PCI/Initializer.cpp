#include <AK/Format.h>

#include <Kernel/Boot/CommandLine.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Bus/PCI/Controller/MemoryBackedHostBridge.h>
#include <Kernel/Bus/PCI/Initializer.h>
#include <Kernel/FileSystem/SysFS/Subsystems/Bus/PCI/BusDirectory.h>

namespace Kernel::PCI {

bool g_pci_access_io_probe_failed { false };
bool g_pci_access_is_disabled_from_commandline { false };

void initialize()
{
    g_pci_access_is_disabled_from_commandline = kernel_command_line().is_pci_disabled();

    if (g_pci_access_is_disabled_from_commandline) {
        return;
    }

    u8 start_bus = 0;
    u8 end_bus = 0xff;

    // Qemu/RVVM
    auto start_addr = PhysicalAddress { 0x3000'0000 };

    // VisionFive 2 pcie1 (NVMe)
    // auto start_addr = PhysicalAddress { 0x2c00'0000 };

    VERIFY(!Access::is_initialized());
    auto* access = new Access();

    Domain pci_domain { 0, start_bus, end_bus };
    dmesgln("PCI: New PCI domain @ {}, PCI buses ({}-{})", PhysicalAddress { start_addr }, start_bus, end_bus);
    auto host_bridge = MemoryBackedHostBridge::must_create(pci_domain, PhysicalAddress { start_addr });
    access->add_host_controller(move(host_bridge));
    access->rescan_hardware();
    dbgln_if(PCI_DEBUG, "PCI: access for multiple PCI domain initialised.");

    PCIBusSysFSDirectory::initialize();

    // // IRQ from pin-based interrupt should be set as reserved as soon as possible so that the PCI device
    // // that chooses to use MSI(x) based interrupt can avoid sharing the IRQ with other devices.
    // MUST(PCI::enumerate([&](DeviceIdentifier const& device_identifier) {
    //     // A simple sanity check to avoid getting a panic in get_interrupt_handler() before setting the IRQ as reserved.
    //     if (auto irq = device_identifier.interrupt_line().value(); irq < GENERIC_INTERRUPT_HANDLERS_COUNT) {
    //         auto& handler = get_interrupt_handler(irq);
    //         handler.set_reserved();
    //     }
    // }));

    MUST(PCI::enumerate([&](DeviceIdentifier const& device_identifier) {
        SpinlockLocker locker(device_identifier.operation_lock());
        dmesgln("{} {}", device_identifier.address(), device_identifier.hardware_id());
        // for (int i = 0; i < 0x40; i += 4) {
        //     dbgln("{:#02x}: {:#08x}", i, PCI::read32_locked(device_identifier, static_cast<RegisterOffset>(i)));
        // }
    }));
}

}
