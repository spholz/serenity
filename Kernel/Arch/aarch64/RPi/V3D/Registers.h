/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

namespace Kernel::RPi::V3D {

struct HubRegisters {
    u8 unknown1[0x4];

    u32 uifcfg; // UIFCFG

    u32 identification_0; // IDENT0
    u32 identification_1; // IDENT1
    u32 identification_2; // IDENT2
    u32 identification_3; // IDENT3

    u8 unknown2[0x3fe8];
};
static_assert(AssertSize<HubRegisters, 0x4000>());

struct CoreRegisters {
    u32 identification_0;
    u32 identification_1;
    u32 identification_2;

    u8 unknown1[0xc];

    u32 misccfg; // MISCCFG

    u8 unknown2[0x34];

    u32 interrupt_status; // INTSTS

    u8 unknown3[0xac];

    // Thread 0: Tile binning
    // Thread 1: Tile rendering

    struct {
        u32 thread_0_control_and_status; // CT0CS
        u32 thread_1_control_and_status; // CT1CS

        u32 thread_0_end_address; // CT0EA
        u32 thread_1_end_address; // CT1EA

        u32 thread_0_current_address; // CT0CA
        u32 thread_1_current_address; // CT1CA

        u32 thread_0_return_address; // CT0RA
        u32 thread_1_return_address; // CT1RA

        u32 thread_0_list_counter; // CT0LC
        u32 thread_1_list_counter; // CT1LC

        u32 thread_0_primitive_list_counter; // CT0PC
        u32 thread_1_primitive_list_counter; // CT1PC

        u32 pipeline_control_and_status; // PCS
        u32 binning_mode_flush_count;    // BFC
        u32 rendering_mode_flush_count;  // RFC

        u8 unknown1[0x20];

        // Set bit 0 to enable the tile state data array.
        u32 thread_0_tile_state_data_array_address; // CT0QTS

        u32 thread_0_control_list_start_address; // CT0QBA
        u32 thread_1_control_list_start_address; // CT1QBA

        u32 thread_0_control_list_end_address; // CT0QEA
        u32 thread_1_control_list_end_address; // CT1QEA

        u32 thread_0_tile_allocation_memory_address; // CT0QMA
        u32 thread_0_tile_allocation_memory_size;    // CT0QMS
    } control_list_executor;

    u8 unknown4[0x188];

    u32 current_address_of_binning_memory_pool;    // BPCA
    u32 remaining_size_of_binning_memory_pool;     // BPCS
    u32 address_of_overspill_binning_memory_block; // BPOA
    u32 size_of_overspill_binning_memory_block;    // BPOS

    u8 unknown5[0xbf4];

    u32 fep_overrun_error_signals;                               // FDBGO
    u32 fep_interface_ready_and_stall_signals__fep_busy_signals; // FDBGB
    u32 fep_interface_ready_signals;                             // FDBGR
    u32 fep_internal_stall_input_signals;                        // FDBGS

    u8 unknown6[0xc];

    u32 miscellaneous_error_signals; // ERRSTAT

    u8 unknown7[0x50dc];
};
static_assert(AssertSize<CoreRegisters, 0x6000>());
static_assert(__builtin_offsetof(CoreRegisters, misccfg) == 0x18);
static_assert(__builtin_offsetof(CoreRegisters, interrupt_status) == 0x50);
static_assert(__builtin_offsetof(CoreRegisters, control_list_executor.rendering_mode_flush_count) == 0x138);
static_assert(__builtin_offsetof(CoreRegisters, control_list_executor.thread_0_tile_state_data_array_address) == 0x15c);
static_assert(__builtin_offsetof(CoreRegisters, control_list_executor.thread_0_control_list_start_address) == 0x160);
static_assert(__builtin_offsetof(CoreRegisters, control_list_executor.thread_1_control_list_end_address) == 0x16c);
static_assert(__builtin_offsetof(CoreRegisters, control_list_executor.thread_0_tile_allocation_memory_address) == 0x170);
static_assert(__builtin_offsetof(CoreRegisters, size_of_overspill_binning_memory_block) == 0x30c);
static_assert(__builtin_offsetof(CoreRegisters, fep_overrun_error_signals) == 0xf04);
static_assert(__builtin_offsetof(CoreRegisters, fep_internal_stall_input_signals) == 0xf10);
static_assert(__builtin_offsetof(CoreRegisters, miscellaneous_error_signals) == 0xf20);

}
