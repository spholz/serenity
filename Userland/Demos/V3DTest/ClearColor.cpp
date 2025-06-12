/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "ClearColor.h"

#include "ControlList.h"
#include "ControlRecords.h"
#include "Job.h"

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

static ControlList generate_binner_control_list(Job& job, size_t target_buffer_width, size_t target_buffer_height)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    u8* control_list_raw = reinterpret_cast<u8*>(map_buffer_object(buffer_object));
    __builtin_memset(control_list_raw, 0, buffer_object.size);

    ControlList control_list(buffer_object, { control_list_raw, buffer_object.size });

    ControlRecord::NumberOfLayers number_of_layers {};
    number_of_layers.number_of_layers_minus_one = 1 - 1;
    control_list.append(number_of_layers);

    ControlRecord::TileBinningModeCfg tile_binning_mode_cfg {};
    tile_binning_mode_cfg.tile_allocation_initial_block_size = 0;
    tile_binning_mode_cfg.tile_allocation_block_size = 0;
    tile_binning_mode_cfg.log2_tile_width = 3;
    tile_binning_mode_cfg.log2_tile_height = 3;
    tile_binning_mode_cfg.width_in_pixels_minus_one = target_buffer_width - 1;
    tile_binning_mode_cfg.height_in_pixels_minus_one = target_buffer_height - 1;
    control_list.append(tile_binning_mode_cfg);

    ControlRecord::FlushVCDCache flush_vcd_cache {};
    control_list.append(flush_vcd_cache);

    ControlRecord::OcclusionQueryCounter occlusion_query_counter {};
    occlusion_query_counter.address = 0;
    control_list.append(occlusion_query_counter);

    ControlRecord::StartTileBinning start_tile_binning {};
    control_list.append(start_tile_binning);

    ControlRecord::Flush flush {};
    control_list.append(flush);

    return control_list;
}

static ControlList generate_tile_list(Job& job, u32 target_buffer_pitch, u32 target_buffer_address)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    u8* control_list_raw = reinterpret_cast<u8*>(map_buffer_object(buffer_object));
    __builtin_memset(control_list_raw, 0, buffer_object.size);

    ControlList control_list(buffer_object, { control_list_raw, buffer_object.size });

    ControlRecord::ImplicitTileCoordinates implicit_tile_coords {};
    control_list.append(implicit_tile_coords);

    ControlRecord::EndOfLoads end_of_loads {};
    control_list.append(end_of_loads);

    ControlRecord::PrimListFormat prim_list_format {};
    prim_list_format.primitive_type = 2;
    prim_list_format.tri_strip_or_fan = 0;
    control_list.append(prim_list_format);

    ControlRecord::SetInstanceID set_instance_id {};
    set_instance_id.instance_id = 0;
    control_list.append(set_instance_id);

    ControlRecord::BranchToImplicitTileList branch_implicit_tile {};
    branch_implicit_tile.tile_list_set_number = 0;
    control_list.append(branch_implicit_tile);

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
    store_tile_buffer_general.height_in_ub_or_stride = target_buffer_pitch;
    store_tile_buffer_general.height = 0;
    store_tile_buffer_general.address = target_buffer_address;
    control_list.append(store_tile_buffer_general);

    ControlRecord::ClearRenderTargets clear_render_targets {};
    control_list.append(clear_render_targets);

    ControlRecord::EndOfTileMarker end_of_tile_marker {};
    control_list.append(end_of_tile_marker);

    ControlRecord::ReturnFromSubList return_from_sub_list {};
    control_list.append(return_from_sub_list);

    return control_list;
}

static ControlList generate_render_control_list(Job& job, u32 target_buffer_pitch, u32 target_buffer_address, size_t target_buffer_width, size_t target_buffer_height, size_t tile_width, size_t tile_height)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    u8* control_list_raw = reinterpret_cast<u8*>(map_buffer_object(buffer_object));
    __builtin_memset(control_list_raw, 0, buffer_object.size);

    ControlList control_list(buffer_object, { control_list_raw, buffer_object.size });

    ControlRecord::TileRenderingModeCfgCommon tile_rendering_mode_cfg_common {};
    tile_rendering_mode_cfg_common.number_of_render_targets_minus_one = 1 - 1;
    tile_rendering_mode_cfg_common.image_width_pixels = target_buffer_width;
    tile_rendering_mode_cfg_common.image_height_pixels = target_buffer_height;
    tile_rendering_mode_cfg_common.multisample_mode_4x = 0;
    tile_rendering_mode_cfg_common.double_buffer_in_non_ms_mode = 0;
    tile_rendering_mode_cfg_common.depth_buffer_disable = 0;
    tile_rendering_mode_cfg_common.early_z_test_and_update_direction = 0;
    tile_rendering_mode_cfg_common.early_z_disable = 1;
    tile_rendering_mode_cfg_common.internal_depth_type = 2;
    tile_rendering_mode_cfg_common.early_depth_stencil_clear = 1;
    tile_rendering_mode_cfg_common.log2_tile_width = 3;
    tile_rendering_mode_cfg_common.log2_tile_height = 3;
    control_list.append(tile_rendering_mode_cfg_common);

    ControlRecord::TileRenderingModeCfgRenderTargetPart1 tile_rendering_mode_cfg_render_target_part1 {};
    tile_rendering_mode_cfg_render_target_part1.render_target_number = 0;
    tile_rendering_mode_cfg_render_target_part1.base_address = 0;
    tile_rendering_mode_cfg_render_target_part1.stride_minus_one = 32 - 1;
    tile_rendering_mode_cfg_render_target_part1.internal_bpp = 0;
    tile_rendering_mode_cfg_render_target_part1.internal_type_and_clamping = 8;
    tile_rendering_mode_cfg_render_target_part1.clear_color_low_bits = 0xb20033;
    control_list.append(tile_rendering_mode_cfg_render_target_part1);

    ControlRecord::TileRenderingModeCfgZSClearValues tile_rendering_mode_cfg_zs_clear_values {};
    tile_rendering_mode_cfg_zs_clear_values.z_clear_value = bit_cast<u32>(0.0f);
    tile_rendering_mode_cfg_zs_clear_values.stencil_clear_value = 0;
    control_list.append(tile_rendering_mode_cfg_zs_clear_values);

    ControlRecord::TileListInitialBlockSize tile_list_initial_block_size {};
    tile_list_initial_block_size.size_of_first_block_in_chained_tile_lists = 0;
    tile_list_initial_block_size.use_auto_chained_tile_lists = 1;
    control_list.append(tile_list_initial_block_size);

    ControlRecord::MulticoreRenderingTileListSetBase multicore_rendering_tile_list_set_base {};
    multicore_rendering_tile_list_set_base.tile_list_set_number = 0;
    multicore_rendering_tile_list_set_base.address = job.tile_alloc_memory_bo.offset >> 6;
    control_list.append(multicore_rendering_tile_list_set_base);

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
    control_list.append(multicore_rendering_supertile_cfg);

    ControlRecord::TileCoordinates tile_coordinates {};
    tile_coordinates.tile_column_number = 0;
    tile_coordinates.tile_row_number = 0;
    control_list.append(tile_coordinates);

    ControlRecord::EndOfLoads end_loads {};
    control_list.append(end_loads);

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
    control_list.append(store_tile_buffer_general);

    ControlRecord::ClearRenderTargets clear_render_targets {};
    control_list.append(clear_render_targets);

    ControlRecord::EndOfTileMarker end_of_tile_marker {};
    control_list.append(end_of_tile_marker);

    ControlRecord::TileCoordinates tile_coordinates_0 {};
    tile_coordinates_0.tile_column_number = 0;
    tile_coordinates_0.tile_row_number = 0;
    control_list.append(tile_coordinates_0);

    ControlRecord::EndOfLoads end_of_loads {};
    control_list.append(end_of_loads);

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
    control_list.append(store_tile_buffer_general_0);

    ControlRecord::EndOfTileMarker end_of_tile_marker_0 {};
    control_list.append(end_of_tile_marker_0);

    ControlRecord::FlushVCDCache flush_vcd_cache {};
    control_list.append(flush_vcd_cache);

    auto tile_list = generate_tile_list(job, target_buffer_pitch, target_buffer_address);

    ControlRecord::StartAddressOfGenericTileList generic_tile_list {};
    generic_tile_list.start = tile_list.bo().offset;
    generic_tile_list.end = tile_list.bo().offset + tile_list.data().size();
    control_list.append(generic_tile_list);

    for (size_t row_number_in_supertiles = 0; row_number_in_supertiles < ceil_div(target_buffer_height, tile_height); row_number_in_supertiles++) {
        for (size_t column_number_in_supertiles = 0; column_number_in_supertiles < ceil_div(target_buffer_width, tile_width); column_number_in_supertiles++) {
            ControlRecord::SupertileCoordinates supertile_coordinates {};
            supertile_coordinates.column_number_in_supertiles = column_number_in_supertiles;
            supertile_coordinates.row_number_in_supertiles = row_number_in_supertiles;
            control_list.append(supertile_coordinates);
        }
    }

    ControlRecord::EndOfRendering end_of_rendering {};
    control_list.append(end_of_rendering);

    return control_list;
}

Job run_clear_color(FlatPtr target_buffer_address, size_t target_buffer_width, size_t target_buffer_height, size_t target_buffer_pitch)
{
    static constexpr size_t TILE_WIDTH = 64;
    static constexpr size_t TILE_HEIGHT = 64;

    Job job;

    job.binner_control_list = generate_binner_control_list(job, target_buffer_width, target_buffer_height);

    job.tile_alloc_memory_bo = create_buffer_object(540672);
    job.bo_handles.append(job.tile_alloc_memory_bo.handle);

    job.render_control_list = generate_render_control_list(job, target_buffer_pitch, target_buffer_address, target_buffer_width, target_buffer_height, TILE_WIDTH, TILE_HEIGHT);

    job.tile_state_data_array_bo = create_buffer_object(20480);
    job.bo_handles.append(job.tile_state_data_array_bo.handle);

    return job;
}

#ifdef KERNEL
}
#endif
