/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>
#include <Kernel/Devices/Storage/NVMe/DataStructures.h>

namespace Kernel::NVMe {

// "The maximum size for the Admin Submission Queue and Admin Completion Queue is defined as 4,096 slots."
static constexpr size_t MAX_ADMIN_QUEUE_SIZE = 4096;

// 5 Admin Command Set

// Figure 142: Opcodes for Admin Commands
enum class AdminOpcode : u8 {
    Identify = 0x06,
};

// 5.2.13 Identify command

// Figure 326: Identify – CNS Values
enum class ControllerOrNamespaceStructure : u32 {
    IdentifyNamespaceDataStructure = 0x00,
    IdentifyControllerDataStructure = 0x01,
    ActiveNamespaceIDList = 0x02,
    IOCommandSetSpecificActiveNamespaceIDList = 0x07,
};

// Figure 327: Command Set Identifiers
enum class CommandSetIdentifier : u32 {
    NVMCommandSet = 0x00,
    KeyValueCommandSet = 0x01,
    ZonedNamespaceCommandSet = 0x02,
    SubsystemLocalMemoryCommandSet = 0x03,
    ComputationalProgramsCommandSet = 0x04,
};

// 5.2.13.2.1 Identify Controller Data Structure (CNS 01h)
struct IdentifyControllerDataStrucutre {
    u16 pci_vendor_id;                                             // VID
    u16 pci_subsystem_vendor_id;                                   // SSVID
    char serial_number[20];                                        // SN
    char model_number[40];                                         // MN
    char firmware_revision[8];                                     // FR
    u8 recommended_arbitration_burst;                              // RAB
    u8 ieee_oui_identifier[3];                                     // IEEE
    u8 controller_multipath_io_and_namespace_sharing_capabilities; // CMIC
    u8 maximum_data_transfer_size;                                 // MDTS
    u16 controller_id;                                             // CNTLID
    u32 version;                                                   // VER
    u32 runtime_d3_resume_latency;                                 // RTD3R
    u32 runtime_d3_entry_latency;                                  // RTD3E
    u32 optional_asynchronous_events_supported;                    // OAES
    u32 controller_attributes;                                     // CTRATT
    u16 read_recovery_levels_supported;                            // RRLS
    u8 boot_partition_capabilities;                                // BPCAP
    u8 _[1];                                                       // Reserved
    u32 nvm_subsystem_shutdown_latency;                            // NSSL
    u8 _[2];                                                       // Reserved
    u8 power_loss_signaling_information;                           // PLSI
    u8 controller_type;                                            // CNTRLTYPE
    u8 fru_globally_unique_identifier[16];                         // FGUID

    // TODO: Add more fields when needed. This structure has *a lot* of fields.
    u8 _[4096 - 128];
};
static_assert(AssertSize<IdentifyControllerDataStrucutre, 4096>());

// 5.2.13.2.2 Active Namespace ID list (CNS 02h)
struct ActiveNamespaceIDList {
    NamespaceList<1024> list;
};
static_assert(AssertSize<ActiveNamespaceIDList, 4096>());

// 5.2.13.2.7 Active Namespace ID list (CNS 02h)
struct IOCommandSetSpecificActiveNamespaceIDList {
    NamespaceList<1024> list;
};
static_assert(AssertSize<IOCommandSetSpecificActiveNamespaceIDList, 4096>());

struct IdentifyCommand : CommandDword0 {
    u32 namespace_identifier; // NSID
    u32 _;                    // CDW2: unused
    u32 _;                    // CDW3: unused
    u32 _;                    // MPTR: unused

    // Figure 322: Identify – Data Pointer
    DataPointer data_pointer; // DPTR

    // Figure 323: Identify – Command Dword 10
    ControllerOrNamespaceStructure controller_or_namespace_structure : 8; // CNS
    u32 : 8;
    u32 controller_identifier : 16; // CNTID

    // Figure 324: Identify – Command Dword 11
    u32 controller_or_namespace_specific_identifier : 16; // CNSSID
    u32 : 8;
    CommandSetIdentifier command_set_identifier : 8; // CSI

    u32 _; // CDW12: unused
    u32 _; // CDW13: unused

    // Figure 325: Identify – Command Dword 14
    u32 uuid_index : 7; // UIDX
    u32 : 25;

    u32 _; // CDW15: unused
};
static_assert(AssertSize<IdentifyCommand, 64>());

}
