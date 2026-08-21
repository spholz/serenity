/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/GenericShorthands.h>
#include <AK/NonnullOwnPtr.h>
#include <Kernel/API/VirGL.h>
#include <LibCore/File.h>
#include <LibCore/System.h>
#include <LibV3DGPU/Device.h>
#include <LibV3DGPU/Image.h>
#include <LibV3DGPU/PrebuiltShaders.h>
#include <LibV3DGPU/Shader.h>

#include "ControlList.h"
#include "ControlRecords.h"

#include <Kernel/API/V3D.h>

int g_v3d_fd = -1;

static constexpr size_t TILE_ALLOC_MEMORY_SIZE = 0x84000;

namespace V3DGPU {

static constexpr size_t TILE_WIDTH = 64;
static constexpr size_t TILE_HEIGHT = 64;

struct VertexData {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

ErrorOr<Device::ShaderStateRecord> Device::generate_shader_state_record(Vector<VertexData> const& vertex_array, Gfx::FloatMatrix4x4 const& model_view_projection_matrix)
{
    auto control_list = TRY(ControlList::create(16 * KiB));

    // -- Uniforms --

    static constexpr size_t UNIFORMS_BUFFER_SIZE = 38 * sizeof(f32);

    auto uniforms_list = TRY(ControlList::create(align_up_to(UNIFORMS_BUFFER_SIZE, PAGE_SIZE)));

    // [struct.unpack('!f', bytes.fromhex(uniform[2:]))[0] for line in uniform_data.splitlines() for uniform in line.split()]

    // Vertex shader uniforms
    u32 vertex_shader_uniforms_address = uniforms_list.buffer_object().address() + uniforms_list.data().size();
    uniforms_list.append(model_view_projection_matrix(0, 0));
    uniforms_list.append(model_view_projection_matrix(1, 0));
    uniforms_list.append(model_view_projection_matrix(2, 0));
    uniforms_list.append(model_view_projection_matrix(3, 0));

    uniforms_list.append(model_view_projection_matrix(0, 1));
    uniforms_list.append(model_view_projection_matrix(1, 1));
    uniforms_list.append(model_view_projection_matrix(2, 1));
    uniforms_list.append(model_view_projection_matrix(3, 1));

    uniforms_list.append(model_view_projection_matrix(0, 2));
    uniforms_list.append(model_view_projection_matrix(1, 2));
    uniforms_list.append(model_view_projection_matrix(2, 2));
    uniforms_list.append(model_view_projection_matrix(3, 2));

    uniforms_list.append(static_cast<float>(m_framebuffer_size.width()) * 0.5f * 64.0f); // Viewport x scale

    uniforms_list.append(model_view_projection_matrix(0, 3));
    uniforms_list.append(model_view_projection_matrix(1, 3));
    uniforms_list.append(model_view_projection_matrix(2, 3));
    uniforms_list.append(model_view_projection_matrix(3, 3));

    uniforms_list.append(-static_cast<float>(m_framebuffer_size.height()) * 0.5f * 64.0f); // Viewport y scale
    uniforms_list.append(0.5f);                                                            // Viewport z scale
    uniforms_list.append(0.5f);                                                            // Viewport z offset

    VERIFY(uniforms_list.data().size() == 0x50);

    // Coordinate shader uniforms
    u32 coordinate_shader_uniforms_address = uniforms_list.buffer_object().address() + uniforms_list.data().size();
    uniforms_list.append(model_view_projection_matrix(0, 0));
    uniforms_list.append(model_view_projection_matrix(1, 0));
    uniforms_list.append(model_view_projection_matrix(2, 0));
    uniforms_list.append(model_view_projection_matrix(3, 0));

    uniforms_list.append(model_view_projection_matrix(0, 1));
    uniforms_list.append(model_view_projection_matrix(1, 1));
    uniforms_list.append(model_view_projection_matrix(2, 1));
    uniforms_list.append(model_view_projection_matrix(3, 1));

    uniforms_list.append(model_view_projection_matrix(0, 2));
    uniforms_list.append(model_view_projection_matrix(1, 2));
    uniforms_list.append(model_view_projection_matrix(2, 2));
    uniforms_list.append(model_view_projection_matrix(3, 2));

    uniforms_list.append(static_cast<float>(m_framebuffer_size.width()) * 0.5f * 64.0f); // Viewport x scale

    uniforms_list.append(model_view_projection_matrix(0, 3));
    uniforms_list.append(model_view_projection_matrix(1, 3));
    uniforms_list.append(model_view_projection_matrix(2, 3));
    uniforms_list.append(model_view_projection_matrix(3, 3));

    uniforms_list.append(-static_cast<float>(m_framebuffer_size.height()) * 0.5f * 64.0f); // Viewport y scale

    VERIFY(uniforms_list.data().size() == UNIFORMS_BUFFER_SIZE);

    // -- Vertex data --

    auto const vertex_data_size_in_bytes = vertex_array.size() * sizeof(vertex_array[0]);

    auto vertex_data_buffer_object = TRY(BufferObject::create(align_up_to(vertex_data_size_in_bytes, PAGE_SIZE)));

    void* vertex_data_buffer_object_data = TRY(vertex_data_buffer_object.map());
    memcpy(vertex_data_buffer_object_data, vertex_array.data(), vertex_data_size_in_bytes);

    // -- Shaders --

    static constexpr size_t FRAGMENT_SHADER_SIZE = FRAGMENT_SHADER.size() * sizeof(FRAGMENT_SHADER[0]);
    static constexpr size_t VERTEX_SHADER_SIZE = VERTEX_SHADER.size() * sizeof(VERTEX_SHADER[0]);
    static constexpr size_t COORDINATE_SHADER_SIZE = COORDINATE_SHADER.size() * sizeof(COORDINATE_SHADER[0]);

    static constexpr size_t SHADERS_BUFFER_SIZE = FRAGMENT_SHADER_SIZE + VERTEX_SHADER_SIZE + COORDINATE_SHADER_SIZE;

    static constexpr size_t SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET = 0;
    static constexpr size_t SHADERS_BUFFER_VERTEX_SHADER_OFFSET = FRAGMENT_SHADER_SIZE;
    static constexpr size_t SHADERS_BUFFER_COORDINATE_SHADER_OFFSET = FRAGMENT_SHADER_SIZE + VERTEX_SHADER_SIZE;

    auto shaders_buffer_object = TRY(BufferObject::create(align_up_to(SHADERS_BUFFER_SIZE, PAGE_SIZE)));

    u8* shaders_buffer_object_data = reinterpret_cast<u8*>(TRY(shaders_buffer_object.map()));
    memcpy(shaders_buffer_object_data + SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET, FRAGMENT_SHADER.data(), FRAGMENT_SHADER_SIZE);
    memcpy(shaders_buffer_object_data + SHADERS_BUFFER_VERTEX_SHADER_OFFSET, VERTEX_SHADER.data(), VERTEX_SHADER_SIZE);
    memcpy(shaders_buffer_object_data + SHADERS_BUFFER_COORDINATE_SHADER_OFFSET, COORDINATE_SHADER.data(), COORDINATE_SHADER_SIZE);

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
        .fragment_shader_code_address = (shaders_buffer_object.address() + SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET) >> 3,
        .fragment_shader_uniforms_address = vertex_shader_uniforms_address,
        .vertex_shader_4_way_threadable = 1,
        .vertex_shader_start_in_final_thread_section = 1,
        .vertex_shader_propagate_nans = 0,
        .vertex_shader_code_address = (shaders_buffer_object.address() + SHADERS_BUFFER_VERTEX_SHADER_OFFSET) >> 3,
        .vertex_shader_uniforms_address = vertex_shader_uniforms_address,
        .coordinate_shader_4_way_threadable = 1,
        .coordinate_shader_start_in_final_thread_section = 1,
        .coordinate_shader_propagate_nans = 0,
        .coordinate_shader_code_address = (shaders_buffer_object.address() + SHADERS_BUFFER_COORDINATE_SHADER_OFFSET) >> 3,
        .coordinate_shader_uniforms_address = coordinate_shader_uniforms_address,
    };
    control_list.append(gl_shader_state_record);

    ControlRecord::GLShaderStateAttributeRecord pos_attribute_record = {
        .address = vertex_data_buffer_object.address(),
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
        .address = static_cast<u32>(vertex_data_buffer_object.address() + (3 * sizeof(float))),
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

    return ShaderStateRecord {
        .control_list = move(control_list),
        .uniforms_list = move(uniforms_list),
        .vertex_data_buffer = move(vertex_data_buffer_object),
        .shaders_buffer = move(shaders_buffer_object),
    };
}

ErrorOr<ControlList> Device::generate_initial_binner_control_list()
{
    auto control_list = TRY(ControlList::create(256 * KiB));

    ControlRecord::NumberOfLayers number_of_layers {};
    number_of_layers.number_of_layers_minus_one = 1 - 1;
    control_list.append(number_of_layers);

    ControlRecord::TileBinningModeCfg tile_binning_mode_cfg {};
    tile_binning_mode_cfg.tile_allocation_initial_block_size = 0;
    tile_binning_mode_cfg.tile_allocation_block_size = 0;
    tile_binning_mode_cfg.log2_tile_width = 3;
    tile_binning_mode_cfg.log2_tile_height = 3;
    tile_binning_mode_cfg.width_in_pixels_minus_one = m_framebuffer_size.width() - 1;
    tile_binning_mode_cfg.height_in_pixels_minus_one = m_framebuffer_size.height() - 1;
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
    clip_window.clip_window_width_in_pixels = m_framebuffer_size.width();
    clip_window.clip_window_height_in_pixels = m_framebuffer_size.height();
    control_list.append(clip_window);

    ControlRecord::PointSize point_size {};
    point_size.point_size = bit_cast<u32>(1.0f);
    control_list.append(point_size);

    ControlRecord::LineWidth line_width {};
    line_width.line_width = bit_cast<u32>(1.0f);
    control_list.append(line_width);

    ControlRecord::ClipperXYScaling clipper_xy_scaling {};
    clipper_xy_scaling.viewport_half_width_in_1_64th_of_pixel = bit_cast<u32>(static_cast<float>(m_framebuffer_size.width()) * 0.5f * 64.0f);
    clipper_xy_scaling.viewport_half_height_in_1_64th_of_pixel = bit_cast<u32>(-static_cast<float>(m_framebuffer_size.height()) * 0.5f * 64.0f);
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
    viewport_offset.fine_x = (m_framebuffer_size.width() / 2) * 256;
    viewport_offset.coarse_x = 0;
    viewport_offset.fine_y = (m_framebuffer_size.height() / 2) * 256;
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

    return control_list;
}

ErrorOr<ControlList> Device::generate_tile_list(u32 target_buffer_pitch, u32 target_buffer_address)
{
    auto control_list = TRY(ControlList::create(16 * KiB));

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

ErrorOr<Device::RenderControlList> Device::generate_render_control_list(u32 target_buffer_pitch, u32 target_buffer_address, size_t tile_width, size_t tile_height, u32 clear_color)
{
    auto control_list = TRY(ControlList::create(16 * KiB));

    ControlRecord::TileRenderingModeCfgCommon tile_rendering_mode_cfg_common {};
    tile_rendering_mode_cfg_common.number_of_render_targets_minus_one = 1 - 1;
    tile_rendering_mode_cfg_common.image_width_pixels = m_framebuffer_size.width();
    tile_rendering_mode_cfg_common.image_height_pixels = m_framebuffer_size.height();
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
    tile_rendering_mode_cfg_render_target_part1.clear_color_low_bits = clear_color;
    control_list.append(tile_rendering_mode_cfg_render_target_part1);

    ControlRecord::TileRenderingModeCfgZSClearValues tile_rendering_mode_cfg_zs_clear_values {};
    tile_rendering_mode_cfg_zs_clear_values.z_clear_value = bit_cast<u32>(1.0f); // XXX: The render control list needs to be updated when the clear depth changes
    tile_rendering_mode_cfg_zs_clear_values.stencil_clear_value = 0;
    control_list.append(tile_rendering_mode_cfg_zs_clear_values);

    ControlRecord::TileListInitialBlockSize tile_list_initial_block_size {};
    tile_list_initial_block_size.size_of_first_block_in_chained_tile_lists = 0;
    tile_list_initial_block_size.use_auto_chained_tile_lists = 1;
    control_list.append(tile_list_initial_block_size);

    ControlRecord::MulticoreRenderingTileListSetBase multicore_rendering_tile_list_set_base {};
    multicore_rendering_tile_list_set_base.tile_list_set_number = 0;
    multicore_rendering_tile_list_set_base.address = m_tile_alloc_memory_bo.address() >> 6;
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

    auto tile_list = TRY(generate_tile_list(target_buffer_pitch, target_buffer_address));

    ControlRecord::StartAddressOfGenericTileList generic_tile_list {};
    generic_tile_list.start = tile_list.buffer_object().address();
    generic_tile_list.end = tile_list.buffer_object().address() + tile_list.data().size();
    control_list.append(generic_tile_list);

    for (int row_number_in_supertiles = 0; row_number_in_supertiles < ceil_div(m_framebuffer_size.height(), tile_height); row_number_in_supertiles++) {
        for (int column_number_in_supertiles = 0; column_number_in_supertiles < ceil_div(m_framebuffer_size.width(), tile_width); column_number_in_supertiles++) {
            ControlRecord::SupertileCoordinates supertile_coordinates {};
            supertile_coordinates.column_number_in_supertiles = column_number_in_supertiles;
            supertile_coordinates.row_number_in_supertiles = row_number_in_supertiles;
            control_list.append(supertile_coordinates);
        }
    }

    ControlRecord::EndOfRendering end_of_rendering {};
    control_list.append(end_of_rendering);

    return RenderControlList {
        .control_list = move(control_list),
        .tile_list = move(tile_list),
    };
}

Device::Device(NonnullOwnPtr<Core::File> gpu_file)
    : m_gpu_file { move(gpu_file) }
{
}

ErrorOr<NonnullOwnPtr<Device>> Device::create(Gfx::IntSize min_size)
{
    static constexpr auto DEVICE_PATH = "/dev/gpu/render0"sv;

    auto file_or_error = Core::File::open(DEVICE_PATH, Core::File::OpenMode::ReadWrite | Core::File::OpenMode::DontCreate);
    if (file_or_error.is_error()) {
        dbgln("LibV3DGPU: Failed to open \"{}\": {}", DEVICE_PATH, file_or_error.error());
        return file_or_error.release_error();
    }

    auto file = file_or_error.release_value();
    g_v3d_fd = file->fd();
    auto device = make<Device>(move(file));

    auto initialize_result = device->initialize_context(min_size);
    if (initialize_result.is_error()) {
        dbgln("LibV3DGPU: Failed to initialize context: {}", initialize_result.error());
        return initialize_result.release_error();
    }

    return device;
}

ErrorOr<void> Device::initialize_context(Gfx::IntSize min_size)
{
    m_framebuffer_size = min_size;

    m_framebuffer = TRY(BufferObject::create(align_up_to(m_framebuffer_size.area() * sizeof(u32), PAGE_SIZE)));
    m_framebuffer_data = TRY(m_framebuffer.map());

    m_binner_control_list = TRY(generate_initial_binner_control_list());

    m_tile_alloc_memory_bo = TRY(BufferObject::create(TILE_ALLOC_MEMORY_SIZE));

    m_render_control_list = TRY(generate_render_control_list(m_framebuffer_size.width() * sizeof(u32), m_framebuffer.address(), TILE_WIDTH, TILE_HEIGHT, m_clear_color));

    m_tile_state_data_array_bo = TRY(BufferObject::create(0x5000));

    return {};
}

GPU::DeviceInfo Device::info() const
{
    return {
        .vendor_name = "SerenityOS",
        .device_name = "VideoCore VII 3D 7.1.7.0",
        .num_texture_units = GPU::NUM_TEXTURE_UNITS,
        .num_lights = 8,
        .max_clip_planes = 6,
        .max_texture_size = 4096,
        .max_texture_lod_bias = 2.f,
        .stencil_bits = sizeof(GPU::StencilType) * 8,
        .supports_npot_textures = true,
        .supports_texture_clamp_to_edge = true,
        .supports_texture_env_add = true,
    };
}

void Device::draw_primitives(GPU::PrimitiveType primitive_type, Vector<GPU::Vertex>& vertices)
{
    auto map_primitive_type = [](GPU::PrimitiveType primitive_type) {
        switch (primitive_type) {
        case GPU::PrimitiveType::Lines:
            return ControlRecord::Primitive::Lines;
        case GPU::PrimitiveType::LineLoop:
            return ControlRecord::Primitive::LineLoop;
        case GPU::PrimitiveType::LineStrip:
            return ControlRecord::Primitive::LineStrip;
        case GPU::PrimitiveType::Points:
            return ControlRecord::Primitive::Points;
        case GPU::PrimitiveType::TriangleFan:
            return ControlRecord::Primitive::TriangleFan;
        case GPU::PrimitiveType::Triangles:
            return ControlRecord::Primitive::Triangles;
        case GPU::PrimitiveType::TriangleStrip:
            return ControlRecord::Primitive::TriangleStrip;
        case GPU::PrimitiveType::Quads:
            return ControlRecord::Primitive::Triangles; // The V3D doesn't support quads, so we convert them manually to triangles.
        }

        VERIFY_NOT_REACHED();
    };

    Vector<VertexData> vertex_array;

    auto convert_vertex = [](GPU::Vertex const& vertex) {
        return VertexData {
            .x = vertex.position.x(),
            .y = vertex.position.y(),
            .z = vertex.position.z(),
            .r = vertex.color.x(),
            .g = vertex.color.y(),
            .b = vertex.color.z(),
        };
    };

    if (primitive_type == GPU::PrimitiveType::Quads) {
        if (vertices.size() < 4)
            return;

        size_t quad_count = vertices.size() / 4;

        vertex_array.ensure_capacity(quad_count * 6);

        for (size_t i = 0; i < vertices.size() - 3; i += 4) {
            vertex_array.append(convert_vertex(vertices[0]));
            vertex_array.append(convert_vertex(vertices[1]));
            vertex_array.append(convert_vertex(vertices[2]));

            vertex_array.append(convert_vertex(vertices[2]));
            vertex_array.append(convert_vertex(vertices[3]));
            vertex_array.append(convert_vertex(vertices[0]));
        }
    } else {
        vertex_array.ensure_capacity(vertices.size());

        for (auto const& vertex : vertices) {
            vertex_array.append(convert_vertex(vertex));
        }
    }

    auto model_view_projection_matrix = m_projection_matrix * m_model_view_matrix;

    auto shader_state_record = generate_shader_state_record(vertex_array, model_view_projection_matrix).release_value_but_fixme_should_propagate_errors();

    auto determine_depth_test_function = [this] {
        if (!m_options.enable_depth_test)
            return ControlRecord::CompareFunction::Always;

        switch (m_options.depth_func) {
        case GPU::DepthTestFunction::Never:
            return ControlRecord::CompareFunction::Never;
        case GPU::DepthTestFunction::Always:
            return ControlRecord::CompareFunction::Always;
        case GPU::DepthTestFunction::Less:
            return ControlRecord::CompareFunction::Less;
        case GPU::DepthTestFunction::LessOrEqual:
            return ControlRecord::CompareFunction::LEqual;
        case GPU::DepthTestFunction::Equal:
            return ControlRecord::CompareFunction::Equal;
        case GPU::DepthTestFunction::NotEqual:
            return ControlRecord::CompareFunction::NotEqual;
        case GPU::DepthTestFunction::GreaterOrEqual:
            return ControlRecord::CompareFunction::GEqual;
        case GPU::DepthTestFunction::Greater:
            return ControlRecord::CompareFunction::Greater;
        }

        VERIFY_NOT_REACHED();
    };

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
    cfg_bits.depth_test_function = determine_depth_test_function();
    cfg_bits.z_updates_enable = m_options.enable_depth_test && m_options.enable_depth_write;
    cfg_bits.stencil_enable = 0;
    cfg_bits.blend_enable = 0;
    cfg_bits.direct3d_point_fill_mode = 0;
    cfg_bits.direct3d_provoking_vertex = 0;
    cfg_bits.z_clipping_mode = 1;
    m_binner_control_list.append(cfg_bits);

    ControlRecord::GLShaderState gl_shader_state {};
    gl_shader_state.number_of_attribute_arrays = 2;
    gl_shader_state.address = shader_state_record.control_list.buffer_object().address() >> 5;
    m_binner_control_list.append(gl_shader_state);

    ControlRecord::VertexArrayPrims vertex_array_prims {};
    vertex_array_prims.mode = map_primitive_type(primitive_type);
    vertex_array_prims.length = vertex_array.size();
    vertex_array_prims.index_of_first_vertex = 0;
    m_binner_control_list.append(vertex_array_prims);

    // Ensure that the data doesn't get freed.
    m_shader_state_records.append(move(shader_state_record));
}

void Device::resize(Gfx::IntSize)
{
    dbgln("V3DGPU::Device::resize(): unimplemented");
}

void Device::clear_color(FloatVector4 const& color)
{
    auto clamped = color.clamped(0.0f, 1.0f);
    auto r = static_cast<u8>(clamped.x() * 255);
    auto g = static_cast<u8>(clamped.y() * 255);
    auto b = static_cast<u8>(clamped.z() * 255);
    auto a = static_cast<u8>(clamped.w() * 255);
    m_clear_color = a << 24 | r << 16 | g << 8 | b;
}

void Device::clear_depth(GPU::DepthType depth)
{
    m_clear_depth = depth;
}

void Device::clear_stencil(GPU::StencilType)
{
    dbgln("V3DGPU::Device::clear_stencil(): unimplemented");
}

void Device::blit_from_color_buffer(Gfx::Bitmap& front_buffer)
{
    ControlRecord::Flush flush {};
    m_binner_control_list.append(flush);

    V3DJob kernel_job = {
        .tile_state_data_array_base_address = m_tile_state_data_array_bo.address(),
        .tile_allocation_memory_base_address = m_tile_alloc_memory_bo.address(),
        .tile_allocation_memory_size = m_tile_alloc_memory_bo.size(),

        .binning_control_list_address = m_binner_control_list.buffer_object().address(),
        .binning_control_list_size = static_cast<u32>(m_binner_control_list.data().size()),

        .rendering_control_list_address = m_render_control_list.control_list.buffer_object().address(),
        .rendering_control_list_size = static_cast<u32>(m_render_control_list.control_list.data().size()),
    };

    auto submit_job_result = Core::System::ioctl(g_v3d_fd, V3D_SUBMIT_JOB, &kernel_job);
    if (submit_job_result.is_error()) {
        dbgln("LibV3DGPU: Job submission failed: {}", submit_job_result.error());

        dbgln("Framebuffer: {}", m_framebuffer);

        dbgln("Tile state data array: {}", m_tile_state_data_array_bo);
        dbgln("Tile alloc memory: {}", m_tile_alloc_memory_bo);

        dbgln("Shader state records:");
        for (auto const& shader_state_record : m_shader_state_records) {
            dbgln("  - Control list: {}", shader_state_record.control_list);
            dbgln("    Uniforms list: {}", shader_state_record.uniforms_list);
            dbgln("    Vertex data buffer: {}", shader_state_record.vertex_data_buffer);
            dbgln("    Shaders buffer: {}", shader_state_record.shaders_buffer);
        }

        dbgln("Binner control list: {}", m_binner_control_list);

        dbgln("Render control list:");
        dbgln("  Control list: {}", m_render_control_list.control_list);
        dbgln("  Tile list: {}", m_render_control_list.tile_list);

        TODO(); // FIXME: Propagate errors.
    }

    VERIFY(front_buffer.pitch() == m_framebuffer_size.width() * sizeof(u32)); // XXX: Add support for other pitches
    VERIFY(front_buffer.data_size() == m_framebuffer_size.area() * sizeof(u32));
    VERIFY(front_buffer.size() == m_framebuffer_size);

#if ARCH(AARCH64)
    auto relatively_fast_copy = [](u8* dest, u8 const* src, size_t n) {
        VERIFY((reinterpret_cast<FlatPtr>(dest) % 16) == 0);
        VERIFY((reinterpret_cast<FlatPtr>(src) % 16) == 0);
        VERIFY((n % 16) == 0);

#    pragma GCC unroll 8
        for (; n > 0; n -= 16) {
            register void* x0 asm("x0") = dest;
            register void const* x1 asm("x1") = src;

            asm volatile(R"(
                ldr q3, [x1]
                str q3, [x0]
            )" ::"r"(x0),
                "r"(x1) : "memory", "q3");

            dest += 16;
            src += 16;
        }
    };

    relatively_fast_copy(front_buffer.scanline_u8(0), reinterpret_cast<u8 const*>(m_framebuffer_data), front_buffer.data_size());
#else
    memcpy(front_buffer.scanline_u8(0), m_framebuffer_data, front_buffer.data_size());
#endif

    m_binner_control_list = generate_initial_binner_control_list().release_value_but_fixme_should_propagate_errors();
    m_shader_state_records.clear();

    // m_clear_color = 0xff00'0000;
    // m_clear_depth = 0.0f;
}

void Device::blit_from_color_buffer(NonnullRefPtr<GPU::Image>, u32, Vector2<u32>, Vector2<i32>, Vector3<i32>)
{
    dbgln("V3DGPU::Device::blit_from_color_buffer(): unimplemented");
}

void Device::blit_from_color_buffer(void*, Vector2<i32>, GPU::ImageDataLayout const&)
{
    dbgln("V3DGPU::Device::blit_from_color_buffer(): unimplemented");
}

void Device::blit_from_depth_buffer(void*, Vector2<i32>, GPU::ImageDataLayout const&)
{
    dbgln("V3DGPU::Device::blit_from_depth_buffer(): unimplemented");
}

void Device::blit_from_depth_buffer(NonnullRefPtr<GPU::Image>, u32, Vector2<u32>, Vector2<i32>, Vector3<i32>)
{
    dbgln("V3DGPU::Device::blit_from_depth_buffer(): unimplemented");
}

void Device::blit_to_color_buffer_at_raster_position(void const*, GPU::ImageDataLayout const&)
{
    dbgln("V3DGPU::Device::blit_to_color_buffer_at_raster_position(): unimplemented");
}

void Device::blit_to_depth_buffer_at_raster_position(void const*, GPU::ImageDataLayout const&)
{
    dbgln("V3DGPU::Device::blit_to_depth_buffer_at_raster_position(): unimplemented");
}

void Device::set_options(GPU::RasterizerOptions const& options)
{
    m_options = options;
}

void Device::set_light_model_params(GPU::LightModelParameters const&)
{
    dbgln("V3DGPU::Device::set_light_model_params(): unimplemented");
}

GPU::RasterizerOptions Device::options() const
{
    return m_options;
}

GPU::LightModelParameters Device::light_model() const
{
    dbgln("V3DGPU::Device::light_model(): unimplemented");
    return {};
}

NonnullRefPtr<GPU::Image> Device::create_image(GPU::PixelFormat const& pixel_format, u32 width, u32 height, u32 depth, u32 max_levels)
{
    dbgln("V3DGPU::Device::create_image(): unimplemented");
    return adopt_ref(*new Image(this, pixel_format, width, height, depth, max_levels));
}

ErrorOr<NonnullRefPtr<GPU::Shader>> Device::create_shader(GPU::IR::Shader const&)
{
    dbgln("V3DGPU::Device::create_shader(): unimplemented");
    return adopt_ref(*new Shader(this));
}

void Device::set_model_view_transform(Gfx::FloatMatrix4x4 const& model_view_transform)
{
    m_model_view_matrix = model_view_transform;
}

void Device::set_projection_transform(Gfx::FloatMatrix4x4 const& projection_transform)
{
    m_projection_matrix = projection_transform;
}

void Device::set_sampler_config(unsigned, GPU::SamplerConfig const&)
{
    dbgln("V3DGPU::Device::set_sampler_config(): unimplemented");
}

void Device::set_light_state(unsigned, GPU::Light const&)
{
    dbgln("V3DGPU::Device::set_light_state(): unimplemented");
}

void Device::set_material_state(GPU::Face, GPU::Material const&)
{
    dbgln("V3DGPU::Device::set_material_state(): unimplemented");
}

void Device::set_stencil_configuration(GPU::Face, GPU::StencilConfiguration const&)
{
    dbgln("V3DGPU::Device::set_stencil_configuration(): unimplemented");
}

void Device::set_texture_unit_configuration(GPU::TextureUnitIndex, GPU::TextureUnitConfiguration const&)
{
    dbgln("V3DGPU::Device::set_texture_unit_configuration(): unimplemented");
}

void Device::set_clip_planes(Vector<FloatVector4> const&)
{
    dbgln("V3DGPU::Device::set_clip_planes(): unimplemented");
}

GPU::RasterPosition Device::raster_position() const
{
    dbgln("V3DGPU::Device::raster_position(): unimplemented");
    return {};
}

void Device::set_raster_position(GPU::RasterPosition const&)
{
    dbgln("V3DGPU::Device::set_raster_position(): unimplemented");
}

void Device::set_raster_position(FloatVector4 const&)
{
    dbgln("V3DGPU::Device::set_raster_position(): unimplemented");
}

void Device::bind_fragment_shader(RefPtr<GPU::Shader>)
{
    dbgln("V3DGPU::Device::bind_fragment_shader(): unimplemented");
}

}

extern "C" GPU::Device* serenity_gpu_create_device(Gfx::IntSize size)
{
    auto device_or_error = V3DGPU::Device::create(size);
    if (device_or_error.is_error())
        return nullptr;

    return device_or_error.release_value().leak_ptr();
}
