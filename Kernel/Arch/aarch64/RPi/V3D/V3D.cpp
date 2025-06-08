/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/aarch64/ASM_wrapper.h>
#include <Kernel/Arch/aarch64/RPi/V3D/ControlList.h>
#include <Kernel/Arch/aarch64/RPi/V3D/ControlRecords.h>
#include <Kernel/Arch/aarch64/RPi/V3D/Registers.h>
#include <Kernel/Arch/aarch64/RPi/V3D/V3D.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel::RPi::V3D {

static void dump_hub_registers(HubRegisters const volatile& registers)
{
    dbgln("V3D Core Registers:");
    dbgln("  UIFCFG: {:#08x}", (u32)registers.uifcfg);
    dbgln("  IDENT0: {:#08x}", (u32)registers.identification_0);
    dbgln("  IDENT1: {:#08x}", (u32)registers.identification_1);
    dbgln("  IDENT2: {:#08x}", (u32)registers.identification_2);
    dbgln("  IDENT3: {:#08x}", (u32)registers.identification_3);
}

static void dump_core_registers(CoreRegisters const volatile& registers)
{
    dbgln("V3D Core Registers:");
    dbgln("  IDENT0: {:#08x}", (u32)registers.identification_0);
    dbgln("  IDENT1: {:#08x}", (u32)registers.identification_1);
    dbgln("  IDENT2: {:#08x}", (u32)registers.identification_2);
    dbgln();
    dbgln("  MISCCFG: {:#08x}", (u32)registers.misccfg);
    dbgln("  INTSTS: {:#08x}", (u32)registers.interrupt_status);
    dbgln();
    dbgln("  PCS: {:#08x}", (u32)registers.control_list_executor.pipeline_control_and_status);
    dbgln("  BFC: {:#08x}", (u32)registers.control_list_executor.binning_mode_flush_count);
    dbgln("  RFC: {:#08x}", (u32)registers.control_list_executor.rendering_mode_flush_count);
    dbgln();
    dbgln("  BPCA: {:#08x}", (u32)registers.current_address_of_binning_memory_pool);
    dbgln("  BPCS: {:#08x}", (u32)registers.remaining_size_of_binning_memory_pool);
    dbgln("  BPOA: {:#08x}", (u32)registers.address_of_overspill_binning_memory_block);
    dbgln("  BPOS: {:#08x}", (u32)registers.size_of_overspill_binning_memory_block);
    dbgln();
    dbgln("  FDBGO: {:#08x}", (u32)registers.fep_overrun_error_signals);
    dbgln("  FDBGB: {:#08x}", (u32)registers.fep_interface_ready_and_stall_signals__fep_busy_signals);
    dbgln("  FDBGR: {:#08x}", (u32)registers.fep_interface_ready_signals);
    dbgln("  FDBGS: {:#08x}", (u32)registers.fep_internal_stall_input_signals);
    dbgln("  ERRSTAT: {:#08x}", (u32)registers.miscellaneous_error_signals);
    dbgln();
    dbgln("  Thread 0:");
    dbgln("    CT0CS: {:#08x}", (u32)registers.control_list_executor.thread_0_control_and_status);
    dbgln("    CT0EA: {:#08x}", (u32)registers.control_list_executor.thread_0_end_address);
    dbgln("    CT0CA: {:#08x}", (u32)registers.control_list_executor.thread_0_current_address);
    dbgln("    CT0RA: {:#08x}", (u32)registers.control_list_executor.thread_0_return_address);
    dbgln("    CT0LC: {:#08x}", (u32)registers.control_list_executor.thread_0_list_counter);
    dbgln("    CT0PC: {:#08x}", (u32)registers.control_list_executor.thread_0_primitive_list_counter);
    dbgln("    CT0QTS: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_state_data_array_address);
    dbgln("    CT0QBA: {:#08x}", (u32)registers.control_list_executor.thread_0_control_list_start_address);
    dbgln("    CT0QEA: {:#08x}", (u32)registers.control_list_executor.thread_0_control_list_end_address);
    dbgln("    CT0QMA: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_alloc_memory_address);
    dbgln("    CT0QMS: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_alloc_memory_size);
    dbgln("  Thread 1:");
    dbgln("    CT1CS: {:#08x}", (u32)registers.control_list_executor.thread_1_control_and_status);
    dbgln("    CT1EA: {:#08x}", (u32)registers.control_list_executor.thread_1_end_address);
    dbgln("    CT1CA: {:#08x}", (u32)registers.control_list_executor.thread_1_current_address);
    dbgln("    CT1RA: {:#08x}", (u32)registers.control_list_executor.thread_1_return_address);
    dbgln("    CT1LC: {:#08x}", (u32)registers.control_list_executor.thread_1_list_counter);
    dbgln("    CT1PC: {:#08x}", (u32)registers.control_list_executor.thread_1_primitive_list_counter);
    dbgln("    CT1QBA: {:#08x}", (u32)registers.control_list_executor.thread_1_control_list_start_address);
    dbgln("    CT1QEA: {:#08x}", (u32)registers.control_list_executor.thread_1_control_list_end_address);
    dbgln();
}

ErrorOr<NonnullRefPtr<V3D>> V3D::create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource)
{
    if (hub_registers_resource.size < sizeof(HubRegisters))
        return EINVAL;

    if (core_0_registers_resource.size < sizeof(CoreRegisters))
        return EINVAL;

    auto hub_registers = TRY(Memory::map_typed_writable<HubRegisters volatile>(hub_registers_resource.paddr));
    auto core_0_registers = TRY(Memory::map_typed_writable<CoreRegisters volatile>(core_0_registers_resource.paddr));

    auto v3d = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) V3D(move(hub_registers), move(core_0_registers))));
    TRY(v3d->initialize());

    return v3d;
}

V3D::V3D(Memory::TypedMapping<HubRegisters volatile> hub_registers, Memory::TypedMapping<CoreRegisters volatile> core_0_registers)
    : m_hub_registers(move(hub_registers))
    , m_core_0_registers(move(core_0_registers))
{
}

ErrorOr<void> V3D::initialize()
{
    dump_hub_registers(*m_hub_registers);
    dump_core_registers(*m_core_0_registers);

    static Array<u8, 16 * KiB> binner_control_list_raw;
    static Array<u8, 16 * KiB> render_control_list_raw;
    static Array<u8, 16 * KiB> tile_control_list_raw;
    static Array<u8, 20480> tile_state_array_memory;
    static Array<u8, 540672> tile_alloc_memory;

    size_t const framebuffer_pitch = g_boot_info.boot_framebuffer.pitch;
    PhysicalPtr const framebuffer_address = g_boot_info.boot_framebuffer.paddr.get();

    PhysicalPtr const tile_alloc_memory_address = Memory::virtual_to_low_physical(bit_cast<FlatPtr>(&tile_alloc_memory));
    PhysicalPtr const tile_list_address = Memory::virtual_to_low_physical(bit_cast<FlatPtr>(&tile_control_list_raw));

    PhysicalPtr const tile_state_array_memory_address = Memory::virtual_to_low_physical(bit_cast<FlatPtr>(&tile_state_array_memory));

    PhysicalPtr const binner_control_list_address = Memory::virtual_to_low_physical(bit_cast<FlatPtr>(&binner_control_list_raw));
    PhysicalPtr const render_control_list_address = Memory::virtual_to_low_physical(bit_cast<FlatPtr>(&render_control_list_raw));

    ControlList binner_control_list(binner_control_list_raw);
    {
        // @buffer CL_0x417d000
        // @format ctrllist  /* [CL_0x417d000+0x00000000] */
        // NUMBER_OF_LAYERS
        //     Number of Layers: 1

        ControlRecord::NumberOfLayers number_of_layers {};
        VERIFY(number_of_layers.opcode == 119);
        number_of_layers.number_of_layers_minus_one = 1 - 1;
        binner_control_list.append(number_of_layers);

        // TILE_BINNING_MODE_CFG
        //     tile allocation initial block size: 0 /* tile allocation initial block size 64b */
        //     tile allocation block size: 0 /* tile allocation block size 64b */
        //     Log2 Tile Width: 3 /* tile width 64 pixels */
        //     Log2 Tile Height: 3 /* tile height 64 pixels */
        //     Width (in pixels): 640
        //     Height (in pixels): 480

        ControlRecord::TileBinningModeCfg tile_binning_mode_cfg {};
        tile_binning_mode_cfg.tile_allocation_initial_block_size = 0;
        tile_binning_mode_cfg.tile_allocation_block_size = 0;
        tile_binning_mode_cfg.log2_tile_width = 3;
        tile_binning_mode_cfg.log2_tile_height = 3;
        tile_binning_mode_cfg.width_in_pixels_minus_one = 640 - 1;
        tile_binning_mode_cfg.height_in_pixels_minus_one = 480 - 1;
        binner_control_list.append(tile_binning_mode_cfg);

        // CLEAR_VCD_CACHE

        ControlRecord::FlushVCDCache flush_vcd_cache {};
        binner_control_list.append(flush_vcd_cache);

        // OCCLUSION_QUERY_COUNTER_ENABLE
        //     address: [null]

        ControlRecord::OcclusionQueryCounter occlusion_query_counter {};
        occlusion_query_counter.address = 0;
        binner_control_list.append(occlusion_query_counter);

        // START_TILE_BINNING

        ControlRecord::StartTileBinning start_tile_binning {};
        binner_control_list.append(start_tile_binning);

        // FLUSH

        ControlRecord::Flush flush {};
        binner_control_list.append(flush);

        // @format blank 16365 /* [CL_0x417d000+0x00000013..0x00003fff] */
    }
    VERIFY(binner_control_list.data().size() == 0x13);

    ControlList tile_control_list(tile_control_list_raw);
    {
        // @buffer CL_0x4181000
        // @format ctrllist  /* [CL_0x4181000+0x00000000] */
        // IMPLICIT_TILE_COORDS

        ControlRecord::ImplicitTileCoordinates implicit_tile_coords {};
        tile_control_list.append(implicit_tile_coords);

        // END_LOADS

        ControlRecord::EndOfLoads end_of_loads {};
        tile_control_list.append(end_of_loads);

        // PRIM_LIST_FORMAT
        //     primitive type: 2 /* List Triangles */
        //     tri strip or fan: 0 /* false */

        ControlRecord::PrimListFormat prim_list_format {};
        prim_list_format.primitive_type = 2;
        prim_list_format.tri_strip_or_fan = 0;
        tile_control_list.append(prim_list_format);

        // SET_INSTANCEID
        //     Instance ID: 0

        ControlRecord::SetInstanceID set_instance_id {};
        set_instance_id.instance_id = 0;
        tile_control_list.append(set_instance_id);

        // BRANCH_IMPLICIT_TILE
        //     tile list set number: 0

        ControlRecord::BranchToImplictiTileList branch_implicit_tile {};
        branch_implicit_tile.tile_list_set_number = 0;
        tile_control_list.append(branch_implicit_tile);

        // STORE
        //     Buffer to Store: 0 /* Render target 0 */
        //     Memory Format: 0 /* Raster */
        //     Flip Y: 0 /* false */
        //     Dither Mode: 0 /* None */
        //     Decimate mode: 0 /* sample 0 */
        //     Output Image Format: 27 /* rgba8 */
        //     Clear buffer being stored: 0 /* false */
        //     Channel Reverse: 0 /* false */
        //     R/B swap: 1 /* true */
        //     Height in UB or Stride: 2560
        //     Height: 0
        //     Address: [winsys_0x4051000+0x00000000] /* 0x04051000 */

        ControlRecord::StoreTileBufferGeneral store_tile_buffer_general {};
        store_tile_buffer_general.buffer_to_store = 0;
        store_tile_buffer_general.memory_format = 0;
        store_tile_buffer_general.flip_y = 0;
        store_tile_buffer_general.dither_mode = 0;
        store_tile_buffer_general.decimate_mode = 0;
        store_tile_buffer_general.output_image_format = 27;
        store_tile_buffer_general.clear_buffer_being_stored = 0;
        store_tile_buffer_general.channel_reverse = 0;
        store_tile_buffer_general.r_b_swap = 1;
        store_tile_buffer_general.height_in_ub_or_stride = framebuffer_pitch;
        store_tile_buffer_general.height = 0;
        store_tile_buffer_general.address = framebuffer_address;
        tile_control_list.append(store_tile_buffer_general);

        // CLEAR_RT

        ControlRecord::ClearRenderTargets clear_render_targets {};
        tile_control_list.append(clear_render_targets);

        // END_TILE

        ControlRecord::EndOfTileMarker end_of_tile_marker {};
        tile_control_list.append(end_of_tile_marker);

        // RETURN

        ControlRecord::ReturnFromSubList return_from_sub_list {};
        tile_control_list.append(return_from_sub_list);

        //
        //
        // @format blank 16357 /* [CL_0x4181000+0x0000001b..0x00003fff] */
    }
    VERIFY(tile_control_list.data().size() == 0x1b);

    PhysicalPtr const tile_list_size = tile_control_list.data().size();

    ControlList render_control_list(render_control_list_raw);
    {
        // @buffer CL_0x4225000
        // @format ctrllist  /* [CL_0x4225000+0x00000000] */
        // TILE_RENDERING_MODE_CFG_COMMON
        //     Number of Render Targets: 1
        //     Image Width (pixels): 640
        //     Image Height (pixels): 480
        //     Multisample Mode (4x): 0 /* false */
        //     Double-buffer in non-ms mode: 0 /* false */
        //     Depth-buffer disable: 0 /* false */
        //     Early-Z Test and Update Direction: 0 /* Early-Z direction LT/LE */
        //     Early-Z disable: 1 /* true */
        //     Internal Depth Type: 2 /* depth_16 */
        //     Early Depth/Stencil Clear: 1 /* true */
        //     Log2 Tile Width: 3 /* tile width 64 pixels */
        //     Log2 Tile Height: 3 /* tile height 64 pixels */

        ControlRecord::TileRenderingModeCfgCommon tile_rendering_mode_cfg_common {};
        tile_rendering_mode_cfg_common.number_of_render_targets_minus_one = 1 - 1;
        tile_rendering_mode_cfg_common.image_width_pixels = 640;
        tile_rendering_mode_cfg_common.image_height_pixels = 480;
        tile_rendering_mode_cfg_common.multisample_mode_4x = 0;
        tile_rendering_mode_cfg_common.double_buffer_in_non_ms_mode = 0;
        tile_rendering_mode_cfg_common.depth_buffer_disable = 0;
        tile_rendering_mode_cfg_common.early_z_test_and_update_direction = 0;
        tile_rendering_mode_cfg_common.early_z_disable = 1;
        tile_rendering_mode_cfg_common.internal_depth_type = 2;
        tile_rendering_mode_cfg_common.early_depth_stencil_clear = 1;
        tile_rendering_mode_cfg_common.log2_tile_width = 3;
        tile_rendering_mode_cfg_common.log2_tile_height = 3;
        render_control_list.append(tile_rendering_mode_cfg_common);

        // TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1
        //     Render Target number: 0
        //     Base Address: 0
        //     Stride: 32
        //     Internal BPP: 0 /* 32 */
        //     Internal Type and Clamping: 8 /* 8 */
        //     Clear Color low bits: 11665459

        ControlRecord::TileRenderingModeCfgRenderTargetPart1 tile_rendering_mode_cfg_render_target_part1 {};
        tile_rendering_mode_cfg_render_target_part1.render_target_number = 0;
        tile_rendering_mode_cfg_render_target_part1.base_address = 0;
        tile_rendering_mode_cfg_render_target_part1.stride_minus_one = 32 - 1;
        tile_rendering_mode_cfg_render_target_part1.internal_bpp = 0;
        tile_rendering_mode_cfg_render_target_part1.internal_type_and_clamping = 8;
        tile_rendering_mode_cfg_render_target_part1.clear_color_low_bits = 0xb20033;
        render_control_list.append(tile_rendering_mode_cfg_render_target_part1);

        // TILE_RENDERING_MODE_CFG_ZS_CLEAR_VALUES
        //     Stencil Clear Value: 0
        //     Z Clear Value: 0.000000

        ControlRecord::TileRenderingModeCfgZSClearValues tile_rendering_mode_cfg_zs_clear_values {};
        tile_rendering_mode_cfg_zs_clear_values.z_clear_value = bit_cast<u32>(0.0f);
        tile_rendering_mode_cfg_zs_clear_values.stencil_clear_value = 0;
        render_control_list.append(tile_rendering_mode_cfg_zs_clear_values);

        // TILE_LIST_INITIAL_BLOCK_SIZE
        //     Size of first block in chained tile lists: 0 /* tile allocation block size 64b */
        //     Use auto-chained tile lists: 1 /* true */

        ControlRecord::TileListInitialBlockSize tile_list_initial_block_size {};
        tile_list_initial_block_size.size_of_first_block_in_chained_tile_lists = 0;
        tile_list_initial_block_size.use_auto_chained_tile_lists = 1;
        render_control_list.append(tile_list_initial_block_size);

        // MULTICORE_RENDERING_TILE_LIST_BASE
        //     Tile List Set Number: 0
        //     address: [tile_alloc_0x4235000+0x00000000] /* 0x04235000 */

        ControlRecord::MulticoreRenderingTileListSetBase multicore_rendering_tile_list_set_base {};
        multicore_rendering_tile_list_set_base.tile_list_set_number = 0;
        multicore_rendering_tile_list_set_base.address = tile_alloc_memory_address >> 6;
        render_control_list.append(multicore_rendering_tile_list_set_base);

        // MULTICORE_RENDERING_SUPERTILE_CFG
        //     Supertile Width in Tiles: 1
        //     Supertile Height in Tiles: 1
        //     Total Frame Width in Supertiles: 10
        //     Total Frame Height in Supertiles: 8
        //     Total Frame Width in Tiles: 10
        //     Total Frame Height in Tiles: 8
        //     Multicore Enable: 0 /* false */
        //     Supertile Raster Order: 0 /* false */
        //     Number of Bin Tile Lists: 1

        ControlRecord::MulticoreRenderingSupertileCfg multicore_rendering_supertile_cfg {};
        multicore_rendering_supertile_cfg.supertile_width_in_tiles_minus_one = 1 - 1;
        multicore_rendering_supertile_cfg.supertile_height_in_tiles_minus_one = 1 - 1;
        multicore_rendering_supertile_cfg.total_frame_width_in_supertiles = 10;
        multicore_rendering_supertile_cfg.total_frame_height_in_supertiles = 8;
        multicore_rendering_supertile_cfg.total_frame_width_in_tiles = 10;
        multicore_rendering_supertile_cfg.total_frame_height_in_tiles = 8;
        multicore_rendering_supertile_cfg.multicore_enable = 0;
        multicore_rendering_supertile_cfg.supertile_raster_order = 0;
        multicore_rendering_supertile_cfg.number_of_bin_tile_lists_minus_one = 1 - 1;
        render_control_list.append(multicore_rendering_supertile_cfg);

        // TILE_COORDS
        //     tile column number: 0
        //     tile row number: 0

        ControlRecord::TileCoordinates tile_coordinates {};
        tile_coordinates.tile_column_number = 0;
        tile_coordinates.tile_row_number = 0;
        render_control_list.append(tile_coordinates);

        // END_LOADS

        ControlRecord::EndOfLoads end_loads {};
        render_control_list.append(end_loads);

        // STORE
        //     Buffer to Store: 8 /* None */
        //     Memory Format: 0 /* Raster */
        //     Flip Y: 0 /* false */
        //     Dither Mode: 0 /* None */
        //     Decimate mode: 0 /* sample 0 */
        //     Output Image Format: 0 /* srgb8_alpha8 */
        //     Clear buffer being stored: 0 /* false */
        //     Channel Reverse: 0 /* false */
        //     R/B swap: 0 /* false */
        //     Height in UB or Stride: 0
        //     Height: 0
        //     Address: [null]

        ControlRecord::StoreTileBufferGeneral store_tile_buffer_general {};
        store_tile_buffer_general.buffer_to_store = 8;
        store_tile_buffer_general.memory_format = 0;
        store_tile_buffer_general.flip_y = 0;
        store_tile_buffer_general.dither_mode = 0;
        store_tile_buffer_general.decimate_mode = 0;
        store_tile_buffer_general.output_image_format = 0;
        store_tile_buffer_general.clear_buffer_being_stored = 0;
        store_tile_buffer_general.channel_reverse = 0;
        store_tile_buffer_general.r_b_swap = 0;
        store_tile_buffer_general.height_in_ub_or_stride = 0;
        store_tile_buffer_general.height = 0;
        store_tile_buffer_general.address = 0;
        render_control_list.append(store_tile_buffer_general);

        // CLEAR_RT

        ControlRecord::ClearRenderTargets clear_render_targets {};
        render_control_list.append(clear_render_targets);

        // END_TILE

        ControlRecord::EndOfTileMarker end_of_tile_marker {};
        render_control_list.append(end_of_tile_marker);

        // TILE_COORDS
        //     tile column number: 0
        //     tile row number: 0

        ControlRecord::TileCoordinates tile_coordinates_0 {};
        tile_coordinates_0.tile_column_number = 0;
        tile_coordinates_0.tile_row_number = 0;
        render_control_list.append(tile_coordinates_0);

        // END_LOADS

        ControlRecord::EndOfLoads end_of_loads {};
        render_control_list.append(end_of_loads);

        // STORE
        //     Buffer to Store: 8 /* None */
        //     Memory Format: 0 /* Raster */
        //     Flip Y: 0 /* false */
        //     Dither Mode: 0 /* None */
        //     Decimate mode: 0 /* sample 0 */
        //     Output Image Format: 0 /* srgb8_alpha8 */
        //     Clear buffer being stored: 0 /* false */
        //     Channel Reverse: 0 /* false */
        //     R/B swap: 0 /* false */
        //     Height in UB or Stride: 0
        //     Height: 0
        //     Address: [null]

        ControlRecord::StoreTileBufferGeneral store_tile_buffer_general_0 {};
        store_tile_buffer_general_0.buffer_to_store = 8;
        store_tile_buffer_general_0.memory_format = 0;
        store_tile_buffer_general_0.flip_y = 0;
        store_tile_buffer_general_0.dither_mode = 0;
        store_tile_buffer_general_0.decimate_mode = 0;
        store_tile_buffer_general_0.output_image_format = 0;
        store_tile_buffer_general_0.clear_buffer_being_stored = 0;
        store_tile_buffer_general_0.channel_reverse = 0;
        store_tile_buffer_general_0.r_b_swap = 0;
        store_tile_buffer_general_0.height_in_ub_or_stride = 0;
        store_tile_buffer_general_0.height = 0;
        store_tile_buffer_general_0.address = 0;
        render_control_list.append(store_tile_buffer_general_0);

        // END_TILE

        ControlRecord::EndOfTileMarker end_of_tile_marker_0 {};
        render_control_list.append(end_of_tile_marker_0);

        // CLEAR_VCD_CACHE

        ControlRecord::FlushVCDCache flush_vcd_cache {};
        render_control_list.append(flush_vcd_cache);

        // GENERIC_TILE_LIST
        //     start: [CL_0x4181000+0x00000000] /* 0x04181000 */
        //     end: [CL_0x4181000+0x0000001b] /* 0x0418101b */

        ControlRecord::StartAddressOfGenericTileList generic_tile_list {};
        generic_tile_list.start = tile_list_address;
        generic_tile_list.end = tile_list_address + tile_list_size;
        render_control_list.append(generic_tile_list);

        // SUPERTILE_COORDS
        //     column number in supertiles: 0
        //     row number in supertiles: 0
        // SUPERTILE_COORDS
        //     column number in supertiles: 1
        //     row number in supertiles: 0
        // [...]
        // SUPERTILE_COORDS
        //     column number in supertiles: 8
        //     row number in supertiles: 7
        // SUPERTILE_COORDS
        //     column number in supertiles: 9
        //     row number in supertiles: 7

        for (u16 row_number_in_supertiles = 0; row_number_in_supertiles < 8; row_number_in_supertiles++) {
            for (u16 column_number_in_supertiles = 0; column_number_in_supertiles < 10; column_number_in_supertiles++) {
                ControlRecord::SupertileCoordinates supertile_coordinates {};
                supertile_coordinates.column_number_in_supertiles = column_number_in_supertiles;
                supertile_coordinates.row_number_in_supertiles = row_number_in_supertiles;
                render_control_list.append(supertile_coordinates);
            }
        }

        // END_RENDER

        ControlRecord::EndOfRendering end_of_rendering {};
        render_control_list.append(end_of_rendering);

        //
        //
        //
        // @format blank 16051 /* [CL_0x4225000+0x0000014d..0x00003fff] */
    }
    VERIFY(render_control_list.data().size() == 0x14d);

    Aarch64::Asm::flush_data_cache(bit_cast<FlatPtr>(binner_control_list_raw.data()), binner_control_list_raw.size());
    Aarch64::Asm::flush_data_cache(bit_cast<FlatPtr>(tile_control_list_raw.data()), tile_control_list_raw.size());
    Aarch64::Asm::flush_data_cache(bit_cast<FlatPtr>(tile_state_array_memory.data()), tile_state_array_memory.size());
    Aarch64::Asm::flush_data_cache(bit_cast<FlatPtr>(tile_alloc_memory.data()), tile_alloc_memory.size());

    asm volatile("isb; dsb sy; isb" ::: "memory");

    m_core_0_registers->control_list_executor.thread_0_tile_alloc_memory_address = tile_alloc_memory_address;
    m_core_0_registers->control_list_executor.thread_0_tile_alloc_memory_size = tile_alloc_memory.size();

    m_core_0_registers->control_list_executor.thread_0_tile_state_data_array_address = tile_state_array_memory_address;

    m_core_0_registers->control_list_executor.thread_0_control_list_start_address = binner_control_list_address;
    m_core_0_registers->control_list_executor.thread_0_control_list_end_address = binner_control_list_address + binner_control_list.data().size();

    dump_core_registers(*m_core_0_registers);

    microseconds_delay(10'000);

    dump_core_registers(*m_core_0_registers);

    Aarch64::Asm::flush_data_cache(bit_cast<FlatPtr>(render_control_list_raw.data()), render_control_list_raw.size());

    asm volatile("isb; dsb sy; isb" ::: "memory");

    m_core_0_registers->control_list_executor.thread_1_control_list_start_address = render_control_list_address;
    m_core_0_registers->control_list_executor.thread_1_control_list_end_address = render_control_list_address + render_control_list.data().size();

    dump_core_registers(*m_core_0_registers);

    microseconds_delay(10'000);

    dump_core_registers(*m_core_0_registers);

    Processor::halt();

    return {};
}

static constinit Array const compatibles_array = {
    "brcm,2712-v3d"sv,
};

DEVICETREE_DRIVER(V3DDriver, compatibles_array);

// https://www.kernel.org/doc/Documentation/devicetree/bindings/gpu/brcm,bcm-v3d.yaml
ErrorOr<void> V3DDriver::probe(DeviceTree::Device const& device, StringView) const
{
    auto hub_registers_resource = TRY(device.get_resource(0));
    auto core_0_registers_resource = TRY(device.get_resource(1));

    (void)TRY(V3D::create(hub_registers_resource, core_0_registers_resource)).leak_ref();

    return {};
}

}
