/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/PCI/Controller/SMCCCHostBridge.h>
#include <Kernel/Arch/aarch64/SMCCC.h>
#include <Kernel/Bus/PCI/Access.h>
#include <Kernel/Memory/MemoryManager.h>
#include <Kernel/Sections.h>

// https://developer.arm.com/documentation/den0115/latest

namespace Kernel::PCI {

enum class PCIConfigAccessFunctionID : u32 {
    Version = 0x8400'0130,        // 2.1 PCI_VERSION
    Features = 0x8400'0131,       // 2.2 PCI_FEATURES
    Read = 0x8400'0132,           // 2.3 PCI_READ
    Write = 0x8400'0133,          // 2.4 PCI_WRITE
    GetSegmentInfo = 0x8400'0134, // 2.5 PCI_GET_SEG_INFO
};

NonnullOwnPtr<SMCCCHostBridge> SMCCCHostBridge::must_create(u16 segment_group_number)
{
    auto register_region = MUST(MM.allocate_mmio_kernel_region(PhysicalAddress { 0x10'0012'0000 }, MUST(Memory::page_round_up(0x9310)), "pcie2"sv, Memory::Region::Access::ReadWrite));
    FlatPtr const register_base = bit_cast<FlatPtr>(register_region->vaddr());

    // auto read8 = [register_base](size_t offset) {
    //     u8 const value = *bit_cast<u8 volatile*>(register_base + offset);
    //     // dbgln("R08 0x100012{:04x} {:#x}", offset, value);
    //     return value;
    // };

    auto read32 = [register_base](size_t offset) {
        u32 const value = *bit_cast<u32 volatile*>(register_base + offset);
        dbgln("R32 0x100012{:04x} {:#x}", offset, value);
        return value;
    };

    auto write16 = [register_base](size_t offset, u16 value) {
        dbgln("W16 0x100012{:04x} {:#x}", offset, value);
        *bit_cast<u16 volatile*>(register_base + offset) = value;
    };

    auto write32 = [register_base](size_t offset, u32 value) {
        dbgln("W32 0x100012{:04x} {:#x}", offset, value);
        *bit_cast<u32 volatile*>(register_base + offset) = value;
    };

    write32(0x4008, 0x38163400);
    read32(0x4008);
    write32(0x404c, 0x0);
    read32(0x404c);
    write32(0x4304, 0x80);
    read32(0x4304);

    write32(0x4064, 0x4); // PERST#
    read32(0x4064);

    read32(0x4068); // contains link status

    write16(0xac + 0x1c, 1 << 4);   // Enable CRS Software Visibility
    write16(0xac + 0x08, 0x2c30);   // Device Control Register
    write16(0x8070 + 0x08, 0x203f); // Device Control Register

    // dbgln("Root Complex Config Space:");
    // for (size_t i = 0; i < 0x1000; i += 8) {
    //     dbgln("{:02x}: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}", i, read8(i + 0), read8(i + 1), read8(i + 2), read8(i + 3), read8(i + 4), read8(i + 5), read8(i + 6), read8(i + 7));
    // }

    // dbgln("RP1 Config Space:");
    // for (size_t i = 0x8000 + 0; i < (0x8000 + 0x1000); i += 8) {
    //     dbgln("{:02x}: {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x} {:02x}", i - 0x8000, read8(i + 0), read8(i + 1), read8(i + 2), read8(i + 3), read8(i + 4), read8(i + 5), read8(i + 6), read8(i + 7));
    // }

    PCI::Domain const domain { segment_group_number, 0, 0xff };
    return adopt_own_if_nonnull(new (nothrow) SMCCCHostBridge(domain)).release_nonnull();
}

SMCCCHostBridge::SMCCCHostBridge(PCI::Domain const& domain)
    : HostController(domain)
{
}

// 2.3 PCI_READ / 2.4 PCI_WRITE
static u32 pci_device_address(u16 segment_group_number, BusNumber bus, DeviceNumber device, FunctionNumber function)
{
    return (segment_group_number << 16u) | (bus.value() << 8u) | (device.value() << 3u) | (function.value() << 0u);
}

// 2.3 PCI_READ
static u32 pci_read(u16 segment_group_number, BusNumber bus, DeviceNumber device, FunctionNumber function, u8 register_offset, u8 access_size)
{
    dbgln("PCI_READ({}, {:#x}, {:#x}, {:#x}, {:#x}, {})", segment_group_number, bus, device, function, register_offset, access_size);

#if 0
    static bool fuse = true;
    if (bus == 1 && fuse) {
        fuse = false;
        asm volatile(R"(
                mov x0, #1
            1:
                cmp x0, #0
                b.ne 1b
        )" ::: "x0");
    }
#endif

    auto result = SMCCC::call32(to_underlying(PCIConfigAccessFunctionID::Read), pci_device_address(segment_group_number, bus, device, function), register_offset, access_size, 0, 0, 0, 0);
    VERIFY(result.w0 == 0); // FIXME: propagate errors
    return result.w1;
}

// 2.4 PCI_WRITE
static void pci_write(u16 segment_group_number, BusNumber bus, DeviceNumber device, FunctionNumber function, u8 register_offset, u8 access_size, u32 value)
{
    dbgln("PCI_WRITE({}, {:#x}, {:#x}, {:#x}, {:#x}, {}, {:#x})", segment_group_number, bus, device, function, register_offset, access_size, value);
    auto result = SMCCC::call32(to_underlying(PCIConfigAccessFunctionID::Write), pci_device_address(segment_group_number, bus, device, function), register_offset, access_size, value, 0, 0, 0);
    VERIFY(result.w0 == 0); // FIXME: propagate errors
}

void SMCCCHostBridge::write8_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u8 value)
{
    VERIFY(m_access_lock.is_locked());
    pci_write(m_domain.domain_number(), bus, device, function, field, 1, value);
}

void SMCCCHostBridge::write16_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u16 value)
{
    VERIFY(m_access_lock.is_locked());
    pci_write(m_domain.domain_number(), bus, device, function, field, 2, value);
}

void SMCCCHostBridge::write32_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field, u32 value)
{
    VERIFY(m_access_lock.is_locked());
    pci_write(m_domain.domain_number(), bus, device, function, field, 4, value);
}

u8 SMCCCHostBridge::read8_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field)
{
    VERIFY(m_access_lock.is_locked());
    return static_cast<u8>(pci_read(2, bus, device, function, field, 1));
}

u16 SMCCCHostBridge::read16_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field)
{
    VERIFY(m_access_lock.is_locked());
    return static_cast<u16>(pci_read(2, bus, device, function, field, 2));
}

u32 SMCCCHostBridge::read32_field_locked(BusNumber bus, DeviceNumber device, FunctionNumber function, u32 field)
{
    VERIFY(m_access_lock.is_locked());
    return pci_read(2, bus, device, function, field, 4);
}

}
