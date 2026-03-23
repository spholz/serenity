/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Endian.h>
#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

namespace Kernel::NVMe {

static_assert(AK::HostIsLittleEndian, "All NVMe data structure definitions assume that the host is little endian");

// 4 Data Structures

// Figure 91: Command Dword 0
// Figure 95: Fabrics Command – Command Dword 0
struct CommandDword0 {
    enum class FusedOperation : u32 {
        NormalOperation = 0b00,
        FirstCommandOfFusedOperation = 0b01,
        SecondCommandOfFusedOperation = 0b10,
    };

    enum class PhysicalRegionPageOrScatterGatherListForDataTransfer : u32 {
        PhysicalRegionPageUsed = 0b00,
        ScatterGatherListUsed_MetadataPointer = 0b01,
        ScatterGatherListUsed_MetadataScatterGatherListSegment = 0b10,
    };

    // Figure 91: Command Dword 0
    u32 opcode : 8;                                                                        // OPC
    FusedOperation fused_operation : 2;                                                    // FUSE
    u32 : 4;                                                                               // Reserved
    PhysicalRegionPageOrScatterGatherListForDataTransfer prp_or_sgl_for_data_transfer : 2; // PSDT
    u32 command_identifier : 16;                                                           // CID
};
static_assert(AssertSize<CommandDword0, 4>());

// Figure 110: PRP Entry – Page Base Address and Offset
struct PhysicalRegionPageEntry {
    u64 page_base_address_and_offset;
};

union DataPointer {
    struct {
        union {
            PhysicalRegionPageEntry physical_region_page_entry;
            u64 physical_region_page_list_pointer;
        } entry_1; // PRP1
        union {
            PhysicalRegionPageEntry physical_region_page_entry;
            u64 physical_region_page_list_pointer;
        } entry_2; // PRP2
    } physical_region_page;
    struct {
        u8 entry_1[16]; // SGL1
    } scatter_gather_list;
};
static_assert(AssertSize<DataPointer, 16>());

// Figure 92: Common Command Format
struct AdminOrIOCommandSubmissionQueueEntry : CommandDword0 {
    u32 namespace_identifier; // NSID
    u32 command_dword2;       // CDW2
    u32 command_dword3;       // CDW3
    u64 metadata_pointer;     // MPTR
    DataPointer data_pointer; // DPTR
    u32 command_dword10;      // CDW10
    u32 command_dword11;      // CDW11
    u32 command_dword12;      // CDW12
    u32 command_dword13;      // CDW13
    u32 command_dword14;      // CDW14
    u32 command_dword15;      // CDW15
};
static_assert(AssertSize<AdminOrIOCommandSubmissionQueueEntry, 64>());

// Figure 96: Common Completion Queue Entry Layout – Admin and All I/O Command Sets
struct AdminOrIOCommandCompletionQueueEntry {
    u8 command_specific[8];
    u32 submission_queue_head_pointer : 16; // SQHD
    u32 submission_queue_identifier : 16;   // SQID
    u32 command_identifier : 16;            // CID
    u32 phase_tag : 1;                      // P
    u32 status : 15;                        // STATUS
};
static_assert(AssertSize<AdminOrIOCommandCompletionQueueEntry, 16>());

// 4.6.2 Namespace List
// This list is zero-terminated.
template<size_t N>
struct NamespaceList {
    u32 namespace_identifier[N];
};

}
