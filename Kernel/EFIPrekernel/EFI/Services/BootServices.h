/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/EFIPrekernel/EFI/EFI.h>

// https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html

namespace EFI {

// EFI_ALLOCATE_TYPE: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-allocatepages
enum class AllocateType {
    AnyPages,
    MaxAddress,
    Address,
};

// EFI_MEMORY_TYPE: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-allocatepages
enum class MemoryType {
    Reserved,
    LoaderCode,
    LoaderData,
    BootServicesCode,
    BootServicesData,
    RuntimeServicesCode,
    RuntimeServicesData,
    Conventional,
    Unusable,
    ACPIReclaim,
    ACPIMemoryNVS,
    MemoryMappedIO,
    MemoryMappedIOPortSpace,
    PalCode,
    Persistent,
    Unaccepted,
};

// EFI_PHYSICAL_ADDRESS: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-allocatepages
using PhysicalAddress = u64;

// EFI_VIRTUAL_ADDRESS: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-getmemorymap
using VirtualAddress = u64;

// EFI_MEMORY_DESCRIPTOR: See "Related Definitions" at https://uefi.org/specs/UEFI/2.10/07_Services_Boot_Services.html#efi-boot-services-getmemorymap
struct MemoryDescriptor {
    u32 type;
    PhysicalAddress physical_start;
    VirtualAddress virtual_start;
    u64 number_of_pages;
    u64 attribute;
};
static_assert(AssertSize<MemoryDescriptor, 40>());

// EFI_BOOT_SERVICES: https://uefi.org/specs/UEFI/2.10/04_EFI_System_Table.html#efi-boot-services-table
struct BootServices {
    static constexpr u64 signature = 0x56524553544f4f42;

    using RaiseTPLFn = void*;
    using RestoreTPLFn = void*;
    using AllocatePagesFn = EFIAPI Status (*)(AllocateType type, MemoryType memory_type, FlatPtr pages, PhysicalAddress* memory);
    using FreePagesFn = EFIAPI Status (*)(PhysicalAddress memory, FlatPtr pages);
    using GetMemoryMapFn = EFIAPI Status (*)(FlatPtr* memory_map_size, MemoryDescriptor* memory_map, FlatPtr* map_key, FlatPtr* descriptor_size, u32* descriptor_version);
    using AllocatePoolFn = EFIAPI Status (*)(MemoryType pool_type, FlatPtr size, void** buffer);
    using FreePoolFn = EFIAPI Status (*)(void* buffer);
    using CreateEventFn = void*;
    using SetTimerFn = void*;
    using WaitForEventFn = void*;
    using SignalEventFn = void*;
    using CloseEventFn = void*;
    using CheckEventFn = void*;
    using InstallProtocolInterfaceFn = void*;
    using ReinstallProtocolInterfaceFn = void*;
    using UninstallProtocolInterfaceFn = void*;
    using HandleProtocolFn = EFIAPI Status (*)(Handle handle, GUID* protocol, void** interface);
    using RegisterProtocolNotifyFn = void*;
    using LocateHandleFn = void*;
    using LocateDevicePathFn = void*;
    using InstallConfigurationTableFn = void*;
    using ImageUnloadFn = void*;
    using ImageStartFn = void*;
    using ExitFn = void*;
    using ExitBootServicesFn = EFIAPI Status (*)(Handle image_handle, FlatPtr map_key);
    using GetNextMonotonicCountFn = void*;
    using StallFn = void*;
    using SetWatchdogTimerFn = void*;
    using ConnectControllerFn = void*;
    using DisconnectControllerFn = void*;
    using OpenProtocolFn = void*;
    using CloseProtocolFn = void*;
    using OpenProtocolInformationFn = void*;
    using ProtocolsPerHandleFn = void*;
    using LocateHandleBufferFn = void*;
    using LocateProtocolFn = EFIAPI Status (*)(GUID* protocol, void* registration, void** interface);
    using UninstallMultipleProtocolInterfacesFn = void*;
    using CalculateCrc32Fn = void*;
    using CopyMemFn = void*;
    using SetMemFn = void*;
    using CreateEventExFn = void*;

    TableHeader hdr;

    // EFI 1.0+

    // Task Priority Services
    RaiseTPLFn raise_tpl;
    RestoreTPLFn restore_tpl;

    // Memory Services
    AllocatePagesFn allocate_pages;
    FreePagesFn free_pages;
    GetMemoryMapFn get_memory_map;
    AllocatePoolFn allocate_pool;
    FreePoolFn free_pool;

    // Event & Timer Services
    CreateEventFn create_event;
    SetTimerFn set_timer;
    WaitForEventFn wait_for_event;
    SignalEventFn signal_event;
    CloseEventFn close_event;
    CheckEventFn check_event;

    // Protocol Handler Services
    InstallProtocolInterfaceFn install_protocol_interface;
    ReinstallProtocolInterfaceFn reinstall_protocol_interface;
    UninstallProtocolInterfaceFn uninstall_protocol_interface;
    HandleProtocolFn handle_protocol;
    void* reserved;
    RegisterProtocolNotifyFn register_protocol_notify;
    LocateHandleFn locate_handle;
    LocateDevicePathFn locate_device_path;
    InstallConfigurationTableFn install_configuration_table;

    // Image Services
    ImageUnloadFn load_image;
    ImageStartFn start_image;
    ExitFn exit;
    ImageUnloadFn unload_image;
    ExitBootServicesFn exit_boot_services;

    // Miscellaneous Services
    GetNextMonotonicCountFn get_next_monotonic_count;
    StallFn stall;
    SetWatchdogTimerFn set_watchdog_timer;

    // EFI 1.1+

    // DriverSupport Services
    ConnectControllerFn connect_controller;
    DisconnectControllerFn disconnect_controller;

    // Open and Close Protocol Services
    OpenProtocolFn open_protocol;
    CloseProtocolFn close_protocol;
    OpenProtocolInformationFn open_protocol_information;

    // Library Services
    ProtocolsPerHandleFn protocols_per_handle;
    LocateHandleBufferFn locate_handle_buffer;
    LocateProtocolFn locate_protocol;
    UninstallMultipleProtocolInterfacesFn install_multiple_protocol_interfaces;
    UninstallMultipleProtocolInterfacesFn uninstall_multiple_protocol_interfaces;

    // 32-bit CRC Services
    CalculateCrc32Fn calculate_crc32;

    // Miscellaneous Services
    CopyMemFn copy_mem;
    SetMemFn set_mem;

    // UEFI 2.0+

    CreateEventExFn create_event_ex;
};
static_assert(AssertSize<BootServices, 376>());

}
