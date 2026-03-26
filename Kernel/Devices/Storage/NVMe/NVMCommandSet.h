/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

namespace Kernel::NVMe {

// Figure 116: LBA Format Data Structure, NVM Command Set Specific
struct NVMCommandSetLBAFormat {
    enum class RelativePerformance : u32 {
        BestPerformance = 0b00,
        BetterPerformance = 0b01,
        GoodPerformance = 0b10,
        DegradedPerformance = 0b11,
    };

    u32 metadata_size : 16;                       // MS
    u32 lba_data_size : 8;                        // LBADS
    RelativePerformance relative_performance : 2; // RP
    u32 : 6;                                      // Reserved
};
static_assert(AssertSize<NVMCommandSetLBAFormat, 4>());

// Figure 114: Identify – Identify Namespace Data Structure, NVM Command Set
struct NVMCommandSetIdentifyNamespaceDataStructure {
    u64 namespace_size;                                            // NSZE
    u64 namespace_capacity;                                        // NCAP
    u64 namespace_utilization;                                     // NUSE
    u8 namespace_features;                                         // NSFEAT
    u8 number_of_lba_formats;                                      // NLBAF
    struct {                                                       //
        u8 format_index_lower : 4;                                 // FIDXL
        u8 metadata_transferred_as_extended_lba : 1;               // MTELBA
        u8 format_index_upper : 2;                                 // FIDXU
        u8 : 1;                                                    // Reserved
    } formatted_lba_size;                                          // FLBAS
    u8 metadata_capabilities;                                      // MC
    u8 end_to_end_data_protection_capabilities;                    // DPC
    u8 end_to_end_data_protection_settings;                        // DPS
    u8 namespace_multi_path_io_and_namespace_sharing_capabilities; // NMIC
    u8 reservation_capabilities;                                   // RESCAP
    u8 format_progress_indicator;                                  // FPI
    u8 deallocate_logical_block_features;                          // DLFEAT
    u16 namespace_atomic_write_unit_normal;                        // NAWUN
    u16 namespace_atomic_write_unit_power_fail;                    // NAWUPF
    u16 namespace_atomic_compare_and_write_unit;                   // NACWU
    u16 namespace_atomic_boundary_size_normal;                     // NABSN
    u16 namespace_atomic_boundary_offset;                          // NABO
    u16 namespace_atomic_boundary_size_power_fail;                 // NABSPF
    u16 namespace_optimal_io_boundary;                             // NOIOB
    u64 nvm_capacity_low;                                          // NVMCAP
    u64 nvm_capacity_high;                                         // NVMCAP
    u16 namespace_preferred_write_granularity;                     // NPWG
    u16 namespace_preferred_write_alignment;                       // NPWA
    u16 namespace_preferred_deallocate_granularity;                // NPDG
    u16 namespace_preferred_deallocate_alignment;                  // NPDA
    u16 namespace_optimal_write_size;                              // NOWS
    u16 maximum_single_source_range_length;                        // MSSRL
    u32 maximum_copy_length;                                       // MCL
    u8 maximum_source_range_count;                                 // MSRC
    u8 key_per_io_status;                                          // KPIOS
    u8 number_of_unique_attribute_lba_formats;                     // NULBAF
    u8 _[127 - 82];                                                // XXX
    NVMCommandSetLBAFormat lba_format_support[64];                 // LBAF0-LBAF63
    u8 _[4095 - 383];                                              // XXX
};
static_assert(AssertSize<NVMCommandSetIdentifyNamespaceDataStructure, 4096>());

}
