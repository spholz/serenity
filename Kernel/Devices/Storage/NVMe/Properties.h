/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/EnumBits.h>
#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

namespace Kernel::NVMe {

// 3.1.4 Controller Properties
enum class PropertyOffset : u32 {
    ControllerCapabilities = 0x0,                          // CAP
    Version = 0x8,                                         // VS
    InterruptMaskSet = 0x0c,                               // INTMS
    InterruptMaskClear = 0x10,                             // INTMC
    ControllerConfiguration = 0x14,                        // CC
    ControllerStatus = 0x1c,                               // CSTS
    NVMSubsystemReset = 0x20,                              // NSSR
    AdminQueueAttributes = 0x24,                           // AQA
    AdminSubmissionQueueBaseAddress = 0x28,                // ASQ
    AdminCompletionQueueBaseAddress = 0x30,                // ACQ
    ControllerMemoryBufferLocation = 0x38,                 // CMBLOC
    ControllerMemoryBufferSize = 0x3c,                     // CMBSZ
    BootPartitionInformation = 0x40,                       // BPINFO
    BootPartitionReadSelect = 0x44,                        // BPRSEL
    BootPartitionMemoryBufferLocation = 0x48,              // BPMBL
    ControllerMemoryBufferMemorySpaceControl = 0x50,       // CMBMSC
    ControllerMemoryBufferStatus = 0x58,                   // CMBSTS
    ControllerMemoryBufferElasticityBufferSize = 0x5c,     // CMBEBS
    ControllerMemoryBufferSustainedWriteThroughput = 0x60, // CMBSWTP
    NVMSubsystemShutdown = 0x64,                           // NSSD
    ControllerReadyTimeouts = 0x68,                        // CRTO

    PersistentMemoryCapabilities = 0xe00,                            // PMRCAP
    PersistentMemoryRegionControl = 0xe04,                           // PMRCTL
    PersistentMemoryRegionStatus = 0xe08,                            // PMRSTS
    PersistentMemoryRegionElasticityBufferSize = 0xe0c,              // PMREBS
    PersistentMemoryRegionSustainedWriteThroughput = 0xe10,          // PMRSWTP
    PersistentMemoryRegionControllerMemorySpaceControlLower = 0xe14, // PMRMSCL
    PersistentMemoryRegionControllerMemorySpaceControlUpper = 0xe18, // PMRMSCU
};

// 3.1.4.1 Offset 0h: CAP – Controller Capabilities
struct ControllerCapabilities {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::ControllerCapabilities;

    enum class CommandSetsSupported : u64 {
        NVMCommandSet = 1 << 0,
        IOCommandSet = 1 << 6,
        NoIOCommandSet = 1 << 7,
    };

    u64 maximum_queue_entries_supported : 16;              // MQES
    u64 contiguous_queues_required : 1;                    // CQR
    u64 arbitration_mechanism_supported : 2;               // AMS
    u64 : 5;                                               // Reserved
    u64 timeout : 8;                                       // TO
    u64 doorbell_stride : 4;                               // DSTRD
    u64 nvm_subsystem_reset_supported : 1;                 // NSSRS
    CommandSetsSupported command_sets_supported : 8;       // CSS
    u64 boot_partition_support : 1;                        // BPS
    u64 controller_power_scope : 2;                        // CPS
    u64 memory_page_size_minimum : 4;                      // MPSMIN
    u64 memory_page_size_maximum : 4;                      // MPSMAX
    u64 persistent_memory_region_supported : 1;            // PMRS
    u64 controller_memory_buffer_supported : 1;            // CMBS
    u64 nvm_subsystem_shutdown_supported : 1;              // NSSS
    u64 controller_ready_modes_supported : 2;              // CRMS
    u64 nvm_subsystem_shutdown_enhancements_supported : 1; // NSSES
    u64 : 2;                                               // Reserved
};
static_assert(AssertSize<ControllerCapabilities, 8>());

AK_ENUM_BITWISE_OPERATORS(ControllerCapabilities::CommandSetsSupported);

// 3.1.4.2 Offset 8h: VS – Version
struct Version {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::Version;

    u32 tertiary_version : 8; // TER
    u32 minor_version : 8;    // MNR
    u32 major_version : 16;   // MJR
};
static_assert(AssertSize<Version, 4>());

// 3.1.4.5 Offset 14h: CC – Controller Configuration
struct ControllerConfiguration {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::ControllerConfiguration;

    enum class ArbitrationMechanismSelected : u32 {
        RoundRobin = 0b000,
        WeightedRoundRobinWithUrgentPriorityClass = 0b001,
    };

    enum class IOCommandSetSelected : u32 {
        NVMCommandSet = 0b000,
        AllSupportedIOCommandSets = 0b110,
        AdminCommandSetOnly = 0b111,
    };

    u32 enable : 1;                                                  // EN
    u32 : 3;                                                         // Reserved
    IOCommandSetSelected io_command_set_selected : 3;                // CSS
    u32 memory_page_size : 4;                                        // MPS
    ArbitrationMechanismSelected arbitration_mechanism_selected : 3; // AMS
    u32 shutdown_notification : 2;                                   // SHN
    u32 io_submission_queue_entry_size : 4;                          // IOSQES
    u32 io_completion_queue_entry_size : 4;                          // IOCQES
    u32 controller_ready_independent_of_media_enable : 1;            // CRIME
    u32 : 7;                                                         // Reserved
};
static_assert(AssertSize<ControllerConfiguration, 4>());

// 3.1.4.6 Offset 1Ch: CSTS – Controller Status
struct ControllerStatus {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::ControllerStatus;

    u32 ready : 1;                       // RDY
    u32 controller_fatal_status : 1;     // CFS
    u32 shutdown_status : 2;             // SHST
    u32 nvm_subsystem_reset_occured : 1; // NSSRO
    u32 processing_paused : 1;           // PP
    u32 shutdown_type : 1;               // ST
    u32 : 25;                            // Reserved
};
static_assert(AssertSize<ControllerStatus, 4>());

// 3.1.4.8 Offset 24h: AQA – Admin Queue Attributes
struct AdminQueueAttributes {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::AdminQueueAttributes;

    u32 admin_submission_queue_size : 12; // ASQS
    u32 : 4;                              // Reserved
    u32 admin_completion_queue_size : 12; // ACQS
    u32 : 4;                              // Reserved
};
static_assert(AssertSize<AdminQueueAttributes, 4>());

// 3.1.4.9 Offset 24h: ASQ – Admin Submission Queue Base Address
struct AdminSubmissionQueueBaseAddress {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::AdminSubmissionQueueBaseAddress;

    u64 admin_submission_queue_base; // ASQB, the lowest 12 bits are reserved
};
static_assert(AssertSize<AdminSubmissionQueueBaseAddress, 8>());

// 3.1.4.10 Offset 24h: ACQ – Admin Completion Queue Base Address
struct AdminCompletionQueueBaseAddress {
    static constexpr auto PROPERTY_OFFSET = PropertyOffset::AdminCompletionQueueBaseAddress;

    u64 admin_completion_queue_base; // ACQB, the lowest 12 bits are reserved
};
static_assert(AssertSize<AdminCompletionQueueBaseAddress, 8>());

}
