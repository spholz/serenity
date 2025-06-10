/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Triangle.h"

#include "ControlList.h"
#include "ControlRecords.h"
#include "Job.h"

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

static ControlList generate_shader_state_record(Job& job, size_t target_buffer_width, size_t target_buffer_height)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    void* control_list_raw = map_buffer_object(buffer_object);
    __builtin_memset(control_list_raw, 0, buffer_object.size);

    ControlList control_list(buffer_object, { control_list_raw, buffer_object.size });

    // -- Uniforms --

    // Vertex shader uniforms
    u32 vertex_shader_uniforms_address = control_list.bo().offset + control_list.data().size();
    control_list.append(static_cast<float>(target_buffer_width) * 0.5f * 64.0f);   // Viewport x scale
    control_list.append(-static_cast<float>(target_buffer_height) * 0.5f * 64.0f); // Viewport y scale
    control_list.append(0.5f);                                                     // Viewport z scale
    control_list.append(0.5f);                                                     // Viewport z offset
    control_list.append(1.0f);

    // Coordinate shader uniforms
    u32 coordinate_shader_uniforms_address = control_list.bo().offset + control_list.data().size();
    control_list.append(static_cast<float>(target_buffer_width) * 0.5f * 64.0f);   // Viewport x scale
    control_list.append(-static_cast<float>(target_buffer_height) * 0.5f * 64.0f); // Viewport y scale
    control_list.append(1.0f);

    // -- Vertex data --

    // clang-format off
    auto vertex_data = to_array<float>({
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,
    });
    // clang-format on

    auto const vertex_data_size_in_bytes = vertex_data.size() * sizeof(vertex_data[0]);

    auto vertex_data_buffer_object = create_buffer_object(vertex_data_size_in_bytes);
    job.bo_handles.append(vertex_data_buffer_object.handle);

    void* vertex_data_buffer_object_data = map_buffer_object(vertex_data_buffer_object);
    __builtin_memcpy(vertex_data_buffer_object_data, vertex_data.data(), vertex_data_size_in_bytes);

    // -- Shaders --

    // Source vertex shader
    // #version 300 es
    //
    // layout(location = 0) in vec3 i_pos;
    // layout(location = 1) in vec3 i_color;
    //
    // out vec3 f_color;
    //
    // void main() {
    //     gl_Position = vec4(i_pos, 1.0);
    //     f_color = i_color;
    // }

    // Source fragment shader
    // #version 300 es
    // precision mediump float;
    //
    // in vec3 f_color;
    //
    // out vec4 o_color;
    //
    // void main() {
    //     o_color = vec4(f_color, 1.0);
    // }

    // clang-format off
    auto shaders_blob = to_array<u32>({
        // Fragment shader
        0xbb03f000, 0x39013186, 0xbb103000, 0x5521d146, 0x051c3005, 0x55228206, 0x05283008, 0x540002c9,
        0x0503f00b, 0x3800218c, 0x3503f189, 0x38203187, 0x3503f328, 0x39e03187, 0xbb03f000, 0x38003186,

        // Vertex shader
        0xbc03f000, 0x39c02185, 0xbc03f040, 0x39c02186, 0xbc03f080, 0x39c02187, 0xbc03f0c0, 0x39c02188,
        0xbb03f000, 0x39813186, 0xbc144100, 0x55c002c9, 0xf503f2c7, 0x3840218d, 0xbc180140, 0x55c0030a,
        0xbe03f108, 0x39c02180, 0xf503f307, 0x3840218f, 0xbe1c0149, 0x55c00380, 0xbe03f18a, 0x39c02180,
        0xbb03f000, 0x38403186, 0xbe03f00d, 0x39c02180, 0x0503f00e, 0x38402190, 0xbe03f04f, 0x39c02180,
        0xbe03f090, 0x39c02180, 0xbe03f0c0, 0x39c02180, 0xbb03f000, 0x38203186, 0xbb03f000, 0x38003186,
        0xbb03f000, 0x38003186,

        // Coordinate shader
                                0xbc03f000, 0x39c02184, 0xbb03f000, 0x38403186, 0xbc100040, 0x55c001c5,
        0xbc03f080, 0x39c02186, 0xf503f1c7, 0x38402189, 0xbe140004, 0x55c00200, 0xbe03f045, 0x39c02180,
        0xf503f207, 0x3840218a, 0xbe03f086, 0x39c02180, 0xbe03f0c0, 0x39c02180, 0xbe03f109, 0x39c02180,
        0xbe03f14a, 0x39c02180, 0xbb03f000, 0x38203186, 0xbb03f000, 0x38003186, 0xbb03f000, 0x38003186,
    });
    // clang-format on

    auto const shaders_blob_size_in_bytes = shaders_blob.size() * sizeof(vertex_data[0]);

    auto shaders_buffer_object = create_buffer_object(shaders_blob_size_in_bytes);
    job.bo_handles.append(shaders_buffer_object.handle);

    void* shaders_buffer_object_data = map_buffer_object(shaders_buffer_object);
    __builtin_memcpy(shaders_buffer_object_data, shaders_blob.data(), shaders_blob_size_in_bytes);

    // -- GL Shader State Record --

    ControlRecord::GLShaderStateRecord gl_shader_state_record = {
        .point_size_in_shaded_vertex_data = 0,
        .enable_clipping = 1,
        .vertex_id_read_by_coordinate_shader = 0,
        .instance_id_read_by_vertex_shader = 0,
        .base_instance_id_read_by_coordinate_shader = 0,
        .vertex_id_read_by_vertex_shader = 0,
        .instance_id_read_by_coordinate_shader = 0,
        .base_instance_id_read_by_vertex_shader = 0,
        .fragment_shader_does_z_writes = 0,
        .turn_off_early_z_test = 0,
        ._reserved0 = 0,
        .fragment_shader_uses_real_pixel_centre_w_in_addition_to_centroid_w2 = 1,
        .enable_sample_rate_shading = 0,
        .any_shader_reads_hardware_written_primitive_id = 0,
        .insert_primitive_id_as_first_varying_to_fragment_shader = 0,
        .turn_off_scoreboard = 0,
        .do_scoreboard_wait_on_first_thread_switch = 0,
        .disable_implicit_point_line_varyings = 1,
        .no_prim_pack = 0,
        .never_defer_fep_depth_writes = 0,
        ._reserved1 = 0,
        .number_of_varyings_in_fragment_shader = 3,
        .coordinate_shader_output_vpm_segment_size = 1,
        .min_coord_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size = 0,
        .coordinate_shader_input_vpm_segment_size = 0,
        .min_coord_shader_input_segments_required_in_play_minus_one = 1,
        .vertex_shader_output_vpm_segment_size = 1,
        .min_vertex_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size = 0,
        .vertex_shader_input_vpm_segment_size = 0,
        .min_vertex_shader_input_segments_required_in_play_minus_one = 1,
        .fragment_shader_4_way_threadable = 1,
        .fragment_shader_start_in_final_thread_section = 0,
        .fragment_shader_propagate_nans = 0,
        .fragment_shader_code_address = (shaders_buffer_object.offset + 0x00) >> 3,
        .fragment_shader_uniforms_address = vertex_shader_uniforms_address,
        .vertex_shader_4_way_threadable = 1,
        .vertex_shader_start_in_final_thread_section = 1,
        .vertex_shader_propagate_nans = 0,
        .vertex_shader_code_address = (shaders_buffer_object.offset + 0x40) >> 3,
        .vertex_shader_uniforms_address = vertex_shader_uniforms_address,
        .coordinate_shader_4_way_threadable = 1,
        .coordinate_shader_start_in_final_thread_section = 1,
        .coordinate_shader_propagate_nans = 0,
        .coordinate_shader_code_address = (shaders_buffer_object.offset + 0xe8) >> 3,
        .coordinate_shader_uniforms_address = coordinate_shader_uniforms_address,
    };
    control_list.append(gl_shader_state_record);

    ControlRecord::GLShaderStateAttributeRecord pos_attribute_record = {
        .address = vertex_data_buffer_object.offset,
        .vec_size = 3,
        .type = 2,
        .signed_int_type = 0,
        .normalized_int_type = 0,
        .read_as_int_uint = 0,
        .number_of_values_read_by_coordinate_shader = 3,
        .number_of_values_read_by_vertex_shader = 3,
        .instance_divisor = 0,
        .stride = 6 * sizeof(float),
        .maximum_index = 0xffffff,
    };
    control_list.append(pos_attribute_record);

    ControlRecord::GLShaderStateAttributeRecord color_attribute_record = {
        .address = static_cast<u32>(vertex_data_buffer_object.offset + (3 * sizeof(float))),
        .vec_size = 3,
        .type = 2,
        .signed_int_type = 0,
        .normalized_int_type = 0,
        .read_as_int_uint = 0,
        .number_of_values_read_by_coordinate_shader = 0,
        .number_of_values_read_by_vertex_shader = 3,
        .instance_divisor = 0,
        .stride = 6 * sizeof(float),
        .maximum_index = 0xffffff,
    };
    control_list.append(color_attribute_record);

    return control_list;
}

static ControlList generate_binner_control_list(Job& job, size_t target_buffer_width, size_t target_buffer_height)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    void* control_list_raw = map_buffer_object(buffer_object);
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

    ControlRecord::ClipWindow clip_window {};
    clip_window.clip_window_left_pixel_coordinate = 0;
    clip_window.clip_window_bottom_pixel_coordinate = 0;
    clip_window.clip_window_width_in_pixels = target_buffer_width;
    clip_window.clip_window_height_in_pixels = target_buffer_height;
    control_list.append(clip_window);

    ControlRecord::CfgBits cfg_bits {};
    cfg_bits.enable_forward_facing_primitive = 1;
    cfg_bits.enable_reverse_facing_primitive = 1;
    cfg_bits.clockwise_primitives = 1;
    cfg_bits.enable_depth_offset = 0;
    cfg_bits.line_rasterization = 0;
    cfg_bits.depth_bounds_test_enable = 0;
    cfg_bits.rasterizer_oversample_mode = 0;
    cfg_bits.z_clamp_mode = 0;
    cfg_bits.direct3d_wireframe_triangles_mode = 0;
    cfg_bits.depth_test_function = 7;
    cfg_bits.z_updates_enable = 0;
    cfg_bits.stencil_enable = 0;
    cfg_bits.blend_enable = 0;
    cfg_bits.direct3d_point_fill_mode = 0;
    cfg_bits.direct3d_provoking_vertex = 0;
    cfg_bits.z_clipping_mode = 1;
    control_list.append(cfg_bits);

    ControlRecord::PointSize point_size {};
    point_size.point_size = bit_cast<u32>(1.0f);
    control_list.append(point_size);

    ControlRecord::LineWidth line_width {};
    line_width.line_width = bit_cast<u32>(1.0f);
    control_list.append(line_width);

    ControlRecord::ClipperXYScaling clipper_xy_scaling {};
    clipper_xy_scaling.viewport_half_width_in_1_64th_of_pixel = bit_cast<u32>(static_cast<float>(target_buffer_width) * 0.5f * 64.0f);
    clipper_xy_scaling.viewport_half_height_in_1_64th_of_pixel = bit_cast<u32>(-static_cast<float>(target_buffer_height) * 0.5f * 64.0f);
    control_list.append(clipper_xy_scaling);

    ControlRecord::ClipperZScaling clipper_z_scaling {};
    clipper_z_scaling.viewport_z_scale = bit_cast<u32>(0.5f);
    clipper_z_scaling.viewport_z_offset = bit_cast<u32>(0.5f);
    control_list.append(clipper_z_scaling);

    ControlRecord::ClipperZMinMaxClippingPlanes clip_z_min_max_clipping_planes {};
    clip_z_min_max_clipping_planes.minimum_zw = bit_cast<u32>(0.0f);
    clip_z_min_max_clipping_planes.maximum_zw = bit_cast<u32>(1.0f);
    control_list.append(clip_z_min_max_clipping_planes);

    ControlRecord::ViewportOffset viewport_offset {};
    viewport_offset.fine_x = (target_buffer_width / 2) * 256;
    viewport_offset.coarse_x = 0;
    viewport_offset.fine_y = (target_buffer_height / 2) * 256;
    viewport_offset.coarse_y = 0;
    control_list.append(viewport_offset);

    ControlRecord::ColorWriteMasks color_write_masks {};
    color_write_masks.mask = 0;
    control_list.append(color_write_masks);

    ControlRecord::BlendConstantColor blend_constant_color {};
    blend_constant_color.red = 0;
    blend_constant_color.green = 0;
    blend_constant_color.blue = 0;
    blend_constant_color.alpha = 0;
    control_list.append(blend_constant_color);

    ControlRecord::ZeroAllFlatShadeFlags zero_all_flat_shade_flags {};
    control_list.append(zero_all_flat_shade_flags);

    ControlRecord::ZeroAllNonPerspectiveFlags zero_all_nonperspective_flags {};
    control_list.append(zero_all_nonperspective_flags);

    ControlRecord::ZeroAllCentroidFlags zero_all_centroid_flags {};
    control_list.append(zero_all_centroid_flags);

    ControlRecord::TransformFeedbackSpecs transform_feedback_specs {};
    transform_feedback_specs.number_of_16bit_output_data_specs_following = 0;
    transform_feedback_specs.enable = 0;
    control_list.append(transform_feedback_specs);

    ControlRecord::OcclusionQueryCounter occlusion_query_counter_1 {};
    occlusion_query_counter_1.address = 0;
    control_list.append(occlusion_query_counter_1);

    ControlRecord::SampleState sample_state {};
    sample_state.mask = 15;
    sample_state.coverage = bit_cast<u32>(1.0f) >> 16;
    control_list.append(sample_state);

    ControlRecord::VCMCacheSize vcm_cache_size {};
    vcm_cache_size.number_of_16_vertex_batches_for_binning = 4;
    vcm_cache_size.number_of_16_vertex_batches_for_rendering = 4;
    control_list.append(vcm_cache_size);

    auto shader_state_record = generate_shader_state_record(job, target_buffer_width, target_buffer_height);

    ControlRecord::GLShaderState gl_shader_state {};
    gl_shader_state.number_of_attribute_arrays = 2;
    gl_shader_state.address = (shader_state_record.bo().offset + 0x20) >> 5;
    control_list.append(gl_shader_state);

    ControlRecord::VertexArrayPrims vertex_array_prims {};
    vertex_array_prims.mode = 4;
    vertex_array_prims.length = 3;
    vertex_array_prims.index_of_first_vertex = 0;
    control_list.append(vertex_array_prims);

    ControlRecord::Flush flush {};
    control_list.append(flush);

    return control_list;
}

static ControlList generate_tile_list(Job& job, u32 target_buffer_pitch, u32 target_buffer_address)
{
    auto buffer_object = create_buffer_object(16 * KiB);
    job.bo_handles.append(buffer_object.handle);

    void* control_list_raw = map_buffer_object(buffer_object);
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

    void* control_list_raw = map_buffer_object(buffer_object);
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
    tile_rendering_mode_cfg_common.early_z_disable = 0;
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

Job run_triangle(FlatPtr target_buffer_address, size_t target_buffer_width, size_t target_buffer_height, size_t target_buffer_pitch)
{
    static constexpr size_t TILE_WIDTH = 64;
    static constexpr size_t TILE_HEIGHT = 64;

    Job job;

    job.binner_control_list = generate_binner_control_list(job, target_buffer_width, target_buffer_height);

    job.tile_alloc_memory_bo = create_buffer_object(0x84000);
    job.bo_handles.append(job.tile_alloc_memory_bo.handle);

    job.render_control_list = generate_render_control_list(job, target_buffer_pitch, target_buffer_address, target_buffer_width, target_buffer_height, TILE_WIDTH, TILE_HEIGHT);

    job.tile_state_data_array_bo = create_buffer_object(0x5000);
    job.bo_handles.append(job.tile_state_data_array_bo.handle);

    return job;
}

#ifdef KERNEL
}
#endif
