/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Device.h"
#include "ControlList.h"
#include "ControlRecords.h"
#include "Definitions.h"
#include "Image.h"
#include "PrebuiltShaders.h"
#include "Shader.h"
#include <AK/NonnullOwnPtr.h>
#include <Kernel/API/V3D.h>
#include <LibCore/File.h>
#include <LibCore/System.h>

// VideoCore IV 3D Architecture Reference Guide: https://docs.broadcom.com/doc/12358545
// This specification only covers an older revision of the VideoCore 3D architecture.

namespace V3DGPU {

int g_v3d_fd = -1;

static constexpr size_t TILE_ALLOC_MEMORY_INITIAL_BLOCK_SIZE = 128;
static constexpr size_t TILE_STATE_DATA_ARRAY_ELEMENT_SIZE = 256;

static constexpr size_t TILE_WIDTH = 64;
static constexpr size_t TILE_HEIGHT = 64;

struct VertexData {
    f32 x;
    f32 y;
    f32 z;
    f32 w;

    f32 r;
    f32 g;
    f32 b;

    f32 u;
    f32 v;
};

[[maybe_unused]] static void dump(ReadonlyBytes data)
{
    auto const* u32s = reinterpret_cast<u32 const*>(data.data());
    auto u32_count = data.size() / sizeof(u32);

    dbgln("{} bytes, {} u32s", data.size(), u32_count);

    for (size_t i = 0; i < ceil_div(u32_count, 8); i++) {
        for (size_t j = 0; j < 8; j++) {
            if (i * 8 + j < u32_count)
                dbg("{:#08x} ", u32s[i * 8 + j]);
        }
        dbgln();
    }
}

ErrorOr<Device::ShaderStateRecord> Device::generate_shader_state_record(Vector<VertexData> const& vertex_array, Gfx::FloatMatrix4x4 const& model_view_projection_matrix)
{
    ControlList control_list;

    // -- Uniforms --

    ControlList uniforms_list;

    // Vertex shader uniforms
    auto vertex_shader_uniforms_offset = uniforms_list.data().size();
    uniforms_list.append<u32>(0);
    uniforms_list.append<u32>(1);
    uniforms_list.append<u32>(2);

    uniforms_list.append(model_view_projection_matrix(0, 0));
    uniforms_list.append(model_view_projection_matrix(1, 0));
    uniforms_list.append(model_view_projection_matrix(2, 0));
    uniforms_list.append(model_view_projection_matrix(3, 0));

    uniforms_list.append(model_view_projection_matrix(0, 1));
    uniforms_list.append(model_view_projection_matrix(1, 1));
    uniforms_list.append(model_view_projection_matrix(2, 1));
    uniforms_list.append(model_view_projection_matrix(3, 1));

    uniforms_list.append<u32>(3);

    uniforms_list.append(model_view_projection_matrix(0, 2));
    uniforms_list.append(model_view_projection_matrix(1, 2));
    uniforms_list.append(model_view_projection_matrix(2, 2));
    uniforms_list.append(model_view_projection_matrix(3, 2));

    uniforms_list.append<u32>(4);
    uniforms_list.append<u32>(5);
    uniforms_list.append<u32>(6);
    uniforms_list.append<u32>(7);
    uniforms_list.append<u32>(8);

    uniforms_list.append(model_view_projection_matrix(0, 3));
    uniforms_list.append(model_view_projection_matrix(1, 3));
    uniforms_list.append(model_view_projection_matrix(2, 3));
    uniforms_list.append(model_view_projection_matrix(3, 3));

    uniforms_list.append(static_cast<f32>(m_framebuffer_size.width()) * 0.5f * 64.0f); // Viewport x scale
    uniforms_list.append<u32>(5);

    uniforms_list.append(-static_cast<f32>(m_framebuffer_size.height()) * 0.5f * 64.0f); // Viewport y scale
    uniforms_list.append<u32>(6);
    uniforms_list.append(0.5f); // Viewport z scale
    uniforms_list.append<u32>(7);
    uniforms_list.append<u32>(8);
    uniforms_list.append(0.5f); // Viewport z offset
    uniforms_list.append<u32>(0);
    uniforms_list.append<u32>(1);
    uniforms_list.append<u32>(2);
    uniforms_list.append<u32>(3);

    // Coordinate shader uniforms
    auto coordinate_shader_uniforms_offset = uniforms_list.data().size();
    uniforms_list.append<u32>(0);
    uniforms_list.append<u32>(1);
    uniforms_list.append<u32>(2);

    uniforms_list.append(model_view_projection_matrix(0, 0));
    uniforms_list.append(model_view_projection_matrix(1, 0));
    uniforms_list.append(model_view_projection_matrix(2, 0));
    uniforms_list.append(model_view_projection_matrix(3, 0));

    uniforms_list.append(model_view_projection_matrix(0, 1));
    uniforms_list.append(model_view_projection_matrix(1, 1));
    uniforms_list.append(model_view_projection_matrix(2, 1));
    uniforms_list.append(model_view_projection_matrix(3, 1));

    uniforms_list.append<u32>(3);

    uniforms_list.append(model_view_projection_matrix(0, 2));
    uniforms_list.append(model_view_projection_matrix(1, 2));
    uniforms_list.append(model_view_projection_matrix(2, 2));
    uniforms_list.append(model_view_projection_matrix(3, 2));

    uniforms_list.append(model_view_projection_matrix(0, 3));
    uniforms_list.append(model_view_projection_matrix(1, 3));
    uniforms_list.append(model_view_projection_matrix(2, 3));
    uniforms_list.append(model_view_projection_matrix(3, 3));

    uniforms_list.append(static_cast<f32>(m_framebuffer_size.width()) * 0.5f * 64.0f);   // Viewport x scale
    uniforms_list.append(-static_cast<f32>(m_framebuffer_size.height()) * 0.5f * 64.0f); // Viewport y scale

    uniforms_list.append<u32>(0);
    uniforms_list.append<u32>(1);
    uniforms_list.append<u32>(2);
    uniforms_list.append<u32>(3);
    uniforms_list.append<u32>(4);
    uniforms_list.append<u32>(5);

    auto const& sampler_configuration = m_sampler_configuration[0].bound_image ? m_sampler_configuration[0] : m_placeholder_sampler_configuration;

    u32 texture_gpu_virtual_address = reinterpret_cast<V3DGPU::Image*>(sampler_configuration.bound_image.ptr())->buffer().gpu_virtual_address();
    u16 texture_width = sampler_configuration.bound_image->width_at_level(0);
    u16 texture_height = sampler_configuration.bound_image->height_at_level(0);

    // u32 texture_gpu_virtual_address = m_sampler_configuration[0].bound_image ? reinterpret_cast<V3DGPU::Image*>(m_sampler_configuration[0].bound_image.ptr())->buffer().gpu_virtual_address() : m_placeholder_texture_buffer->gpu_virtual_address();
    // u16 texture_width = m_sampler_configuration[0].bound_image ? m_sampler_configuration[0].bound_image->width_at_level(0) : 1;
    // u16 texture_height = m_sampler_configuration[0].bound_image ? m_sampler_configuration[0].bound_image->height_at_level(0) : 1;

    // dbgln("texture @ {:#x} ({} bytes)", texture_gpu_virtual_address, texture_width * texture_height * 4);

    ControlList texture_list;

    auto texture_shader_state_offset = texture_list.data().size();
    ControlRecord::TextureShaderState texture_shader_state {
        .flip_texture_x_axis = false,
        .flip_texture_y_axis = false,
        .transfer_func = ControlRecord::TextureShaderState::TransferFunc::None,
        .reverse = 0,
        .texture_base_pointer = texture_gpu_virtual_address >> 6u,
        .r_b_swap = false,
        .array_stride_64_byte_aligned = 0,
        ._reserved0 = 0,
        .image_width = texture_width,
        .image_height = texture_height,
        .image_depth = 1,
        .texture_type = 4,
        .extended = true,
        .swizzle_r = 2,
        .swizzle_g = 3,
        .swizzle_b = 4,
        .swizzle_a = ControlRecord::TextureShaderState::SwizzleA::Alpha,
        .max_level = 0,
        .base_level = 0,
        .level_0_ub_pad = 0,
        .level_0_xor_enable = false,
        ._reserved1 = 0,
        .level_0_is_strictly_uif = false,
        .uif_xor_disable = true,
        .chroma_offset_x = 1,
        .chroma_offset_y = 1,
        .texture_base_pointer_cb = texture_gpu_virtual_address >> 6u,
        .texture_base_pointer_cr = texture_gpu_virtual_address >> 6u,
        .pad = 0,
    };
    texture_list.append(texture_shader_state);

    auto map_wrap_mode = [](GPU::TextureWrapMode wrap_mode) {
        switch (wrap_mode) {
        case GPU::TextureWrapMode::Repeat:
            return ControlRecord::WrapMode::Repeat;
        case GPU::TextureWrapMode::MirroredRepeat:
            return ControlRecord::WrapMode::Mirror;
        case GPU::TextureWrapMode::Clamp:
            return ControlRecord::WrapMode::Clamp;
        case GPU::TextureWrapMode::ClampToBorder:
            return ControlRecord::WrapMode::Border;
        case GPU::TextureWrapMode::ClampToEdge:
            return ControlRecord::WrapMode::Clamp;
        }

        VERIFY_NOT_REACHED();
    };

    auto sampler_state_offset = texture_list.data().size();
    ControlRecord::SamplerState sampler_state {
        .mag_filter_nearest = sampler_configuration.texture_min_filter == GPU::TextureFilter::Nearest,
        .min_filter_nearest = sampler_configuration.texture_min_filter == GPU::TextureFilter::Nearest,
        .mip_filter_nearest = false,
        .anisotropy_enable = false,
        .depth_compare_function = ControlRecord::CompareFunction::Never,
        .srgb_disable = false,
        .min_level_of_detail = 0,
        .max_level_of_detail = 0xf00,
        .fixed_bias = 0,
        .wrap_s = map_wrap_mode(sampler_configuration.texture_wrap_u),
        .wrap_t = map_wrap_mode(sampler_configuration.texture_wrap_v),
        .wrap_r = map_wrap_mode(sampler_configuration.texture_wrap_w),
        .wrap_i_border = false,
        .border_color_mode = ControlRecord::BorderColorMode::Mode0000,
        .maximum_anisotropy = 0,
        .border_color_word_0 = 0,
        .border_color_word_1 = 0,
        .border_color_word_2 = 0,
        .border_color_word_3 = 0,
    };
    texture_list.append(sampler_state);

    TRY(texture_list.copy_to_gpu_buffer());
    // dbgln("texture list @ {:#x}", texture_list.buffer()->gpu_virtual_address());
    // dump(texture_list.data());
    u32 texture_shader_state_address = texture_list.buffer()->gpu_virtual_address() + texture_shader_state_offset;
    u32 sampler_state_address = texture_list.buffer()->gpu_virtual_address() + sampler_state_offset;

    // Fragment shader uniforms
    auto fragment_shader_uniforms_offset = uniforms_list.data().size();

    ControlRecord::TMUConfigParameter0 tmu_config_parameter_0 {
        .return_words_of_texture_data = 3,
        .texture_state_address = texture_shader_state_address >> 4u,
    };
    uniforms_list.append(tmu_config_parameter_0);

    ControlRecord::TMUConfigParameter1 tmu_config_parameter_1 {
        .output_type_32_bit = false,
        .unnormalized_coordinates = false,
        .per_pixel_mask_enable = false,
        .sampler_state_address = sampler_state_address >> 3u,
    };
    uniforms_list.append(tmu_config_parameter_1);

    if (m_options.enable_alpha_test) {
        if (m_options.alpha_test_func != GPU::AlphaTestFunction::Greater) {
            dbgln("LibV3DGPU: Unimplemented alpha test function: {}", to_underlying(m_options.alpha_test_func));
        } else {
            uniforms_list.append<f32>(m_options.alpha_test_ref_value);
            uniforms_list.append<u32>(0xffff'ff84);
        }
    }

    TRY(uniforms_list.copy_to_gpu_buffer());
    // dbgln("uniforms list @ {:#x}", uniforms_list.buffer()->gpu_virtual_address());
    // dump(uniforms_list.data());
    ControlRecord::Address vertex_shader_uniforms_address = uniforms_list.buffer()->gpu_virtual_address() + vertex_shader_uniforms_offset;
    ControlRecord::Address coordinate_shader_uniforms_address = uniforms_list.buffer()->gpu_virtual_address() + coordinate_shader_uniforms_offset;
    ControlRecord::Address fragment_shader_uniforms_address = uniforms_list.buffer()->gpu_virtual_address() + fragment_shader_uniforms_offset;

    // -- Vertex data --

    auto const vertex_data_size_in_bytes = vertex_array.size() * sizeof(vertex_array[0]);

    auto vertex_data_buffer = TRY(Buffer::create(align_up_to(vertex_data_size_in_bytes, V3D_PAGE_SIZE), "LibV3DGPU: Vertex Data Buffer"sv));

    u8* vertex_data_buffer_data = vertex_data_buffer.data().data();
    memcpy(vertex_data_buffer_data, vertex_array.data(), vertex_data_size_in_bytes);

    // dbgln("vertex data @ {:#x}", vertex_data_buffer.gpu_virtual_address());
    // dump(vertex_data_buffer.data().trim(vertex_data_size_in_bytes));

    // -- Shaders --

    bool use_alpha_test_fragment_shader = m_options.enable_alpha_test && m_options.alpha_test_func == GPU::AlphaTestFunction::Greater;
    auto& fragment_shader = use_alpha_test_fragment_shader ? FRAGMENT_SHADER_ALPHA_TEST_GREATER : FRAGMENT_SHADER;

    size_t const fragment_shader_size = fragment_shader.size() * sizeof(fragment_shader[0]);
    static constexpr size_t VERTEX_SHADER_SIZE = VERTEX_SHADER.size() * sizeof(VERTEX_SHADER[0]);
    static constexpr size_t COORDINATE_SHADER_SIZE = COORDINATE_SHADER.size() * sizeof(COORDINATE_SHADER[0]);

    size_t const shaders_buffer_size = fragment_shader_size + VERTEX_SHADER_SIZE + COORDINATE_SHADER_SIZE;

    static constexpr size_t SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET = 0;
    size_t const shaders_buffer_vertex_shader_offset = fragment_shader_size;
    size_t const shaders_buffer_coordinate_shader_offset = fragment_shader_size + VERTEX_SHADER_SIZE;

    auto shaders_buffer = TRY(Buffer::create(align_up_to(shaders_buffer_size, V3D_PAGE_SIZE), "LibV3DGPU: Shaders Buffer"sv));

    u8* shaders_buffer_data = shaders_buffer.data().data();
    memcpy(shaders_buffer_data + SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET, fragment_shader.data(), fragment_shader_size);
    memcpy(shaders_buffer_data + shaders_buffer_vertex_shader_offset, VERTEX_SHADER.data(), VERTEX_SHADER_SIZE);
    memcpy(shaders_buffer_data + shaders_buffer_coordinate_shader_offset, COORDINATE_SHADER.data(), COORDINATE_SHADER_SIZE);

    // -- GL Shader State Record --

    ControlRecord::GLShaderStateRecord gl_shader_state_record = {
        .point_size_in_shaded_vertex_data = false,
        .enable_clipping = true,
        .vertex_id_read_by_coordinate_shader = false,
        .instance_id_read_by_coordinate_shader = false,
        .base_instance_id_read_by_coordinate_shader = false,
        .vertex_id_read_by_vertex_shader = false,
        .instance_id_read_by_vertex_shader = false,
        .base_instance_id_read_by_vertex_shader = false,
        .fragment_shader_does_z_writes = use_alpha_test_fragment_shader,
        .turn_off_early_z_test = use_alpha_test_fragment_shader,
        ._reserved0 = 0,
        .fragment_shader_uses_real_pixel_centre_w_in_addition_to_centroid_w2 = true,
        .enable_sample_rate_shading = false,
        .any_shader_reads_hardware_written_primitive_id = false,
        .insert_primitive_id_as_first_varying_to_fragment_shader = false,
        .turn_off_scoreboard = false,
        .do_scoreboard_wait_on_first_thread_switch = false,
        .disable_implicit_point_line_varyings = true,
        .no_prim_pack = 0,
        .never_defer_fep_depth_writes = false,
        ._reserved1 = 0,
        .number_of_varyings_in_fragment_shader = 5,
        .coordinate_shader_output_vpm_segment_size = 1,
        .min_coord_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size = 0,
        .coordinate_shader_input_vpm_segment_size = 0,
        .min_coord_shader_input_segments_required_in_play_minus_one = 1,
        .vertex_shader_output_vpm_segment_size = 2,
        .min_vertex_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size = 0,
        .vertex_shader_input_vpm_segment_size = 0,
        .min_vertex_shader_input_segments_required_in_play_minus_one = 1,
        .fragment_shader_4_way_threadable = true,
        .fragment_shader_start_in_final_thread_section = false,
        .fragment_shader_propagate_nans = false,
        .fragment_shader_code_address = static_cast<ControlRecord::Address>((shaders_buffer.gpu_virtual_address() + SHADERS_BUFFER_FRAGMENT_SHADER_OFFSET) >> 3u),
        .fragment_shader_uniforms_address = fragment_shader_uniforms_address,
        .vertex_shader_4_way_threadable = true,
        .vertex_shader_start_in_final_thread_section = true,
        .vertex_shader_propagate_nans = false,
        .vertex_shader_code_address = static_cast<ControlRecord::Address>((shaders_buffer.gpu_virtual_address() + shaders_buffer_vertex_shader_offset) >> 3u),
        .vertex_shader_uniforms_address = vertex_shader_uniforms_address,
        .coordinate_shader_4_way_threadable = true,
        .coordinate_shader_start_in_final_thread_section = true,
        .coordinate_shader_propagate_nans = false,
        .coordinate_shader_code_address = static_cast<ControlRecord::Address>((shaders_buffer.gpu_virtual_address() + shaders_buffer_coordinate_shader_offset) >> 3u),
        .coordinate_shader_uniforms_address = coordinate_shader_uniforms_address,
    };
    control_list.append(gl_shader_state_record);

    ControlRecord::GLShaderStateAttributeRecord pos_attribute_record = {
        .address = vertex_data_buffer.gpu_virtual_address(),
        .vec_size = 0, // 0 means 4.
        .type = ControlRecord::GLShaderStateAttributeRecord::Type::AttributeFloat,
        .signed_int_type = false,
        .normalized_int_type = false,
        .read_as_int_uint = false,
        .number_of_values_read_by_coordinate_shader = 4,
        .number_of_values_read_by_vertex_shader = 4,
        .instance_divisor = 0,
        .stride = 9 * sizeof(f32),
        .maximum_index = 0xffffff,
    };
    control_list.append(pos_attribute_record);

    ControlRecord::GLShaderStateAttributeRecord color_attribute_record = {
        .address = static_cast<ControlRecord::Address>(vertex_data_buffer.gpu_virtual_address() + (4 * sizeof(f32))),
        .vec_size = 3,
        .type = ControlRecord::GLShaderStateAttributeRecord::Type::AttributeFloat,
        .signed_int_type = false,
        .normalized_int_type = false,
        .read_as_int_uint = false,
        .number_of_values_read_by_coordinate_shader = 0,
        .number_of_values_read_by_vertex_shader = 3,
        .instance_divisor = 0,
        .stride = 9 * sizeof(f32),
        .maximum_index = 0xffffff,
    };
    control_list.append(color_attribute_record);

    ControlRecord::GLShaderStateAttributeRecord tex_coord_attribute_record = {
        .address = static_cast<ControlRecord::Address>(vertex_data_buffer.gpu_virtual_address() + (7 * sizeof(f32))),
        .vec_size = 2,
        .type = ControlRecord::GLShaderStateAttributeRecord::Type::AttributeFloat,
        .signed_int_type = false,
        .normalized_int_type = false,
        .read_as_int_uint = false,
        .number_of_values_read_by_coordinate_shader = 0,
        .number_of_values_read_by_vertex_shader = 2,
        .instance_divisor = 0,
        .stride = 9 * sizeof(f32),
        .maximum_index = 0xffffff,
    };
    control_list.append(tex_coord_attribute_record);

    // NOTE: If you add another attribute record here, you need to modify GLShaderState::number_of_attribute_arrays.

    TRY(control_list.copy_to_gpu_buffer());

    return ShaderStateRecord {
        .control_list = move(control_list),
        .uniforms_list = move(uniforms_list),
        .texture_list = move(texture_list),
        .vertex_data_buffer = move(vertex_data_buffer),
        .shaders_buffer = move(shaders_buffer),
    };
}

ErrorOr<ControlList> Device::generate_initial_binner_control_list()
{
    ControlList control_list;

    ControlRecord::NumberOfLayers number_of_layers {};
    number_of_layers.number_of_layers_minus_one = 1 - 1;
    control_list.append(number_of_layers);

    ControlRecord::TileBinningModeCfg tile_binning_mode_cfg {};
    static_assert(TILE_ALLOC_MEMORY_INITIAL_BLOCK_SIZE == 128);
    static_assert(TILE_WIDTH == 64);
    static_assert(TILE_HEIGHT == 64);
    tile_binning_mode_cfg.tile_allocation_initial_block_size = ControlRecord::TileBinningModeCfg::TileAllocationInitialBlockSize::Size128Bytes;
    tile_binning_mode_cfg.tile_allocation_block_size = ControlRecord::TileBinningModeCfg::TileAllocationBlockSize::Size64Bytes;
    tile_binning_mode_cfg.log2_tile_width = ControlRecord::TileBinningModeCfg::Log2TileWidth::Width64Pixels;
    tile_binning_mode_cfg.log2_tile_height = ControlRecord::TileBinningModeCfg::Log2TileHeight::Height64Pixels;
    tile_binning_mode_cfg.width_in_pixels_minus_one = m_framebuffer_size.width() - 1;
    tile_binning_mode_cfg.height_in_pixels_minus_one = m_framebuffer_size.height() - 1;
    control_list.append(tile_binning_mode_cfg);

    ControlRecord::FlushVCDCache flush_vcd_cache {};
    control_list.append(flush_vcd_cache);

    ControlRecord::StartTileBinning start_tile_binning {};
    control_list.append(start_tile_binning);

    ControlRecord::ClipWindow clip_window {};
    clip_window.clip_window_left_pixel_coordinate = 0;
    clip_window.clip_window_bottom_pixel_coordinate = 0;
    clip_window.clip_window_width_in_pixels = m_framebuffer_size.width();
    clip_window.clip_window_height_in_pixels = m_framebuffer_size.height();
    control_list.append(clip_window);

    ControlRecord::ClipperXYScaling clipper_xy_scaling {};
    clipper_xy_scaling.viewport_half_width_in_1_64th_of_pixel = static_cast<f32>(m_framebuffer_size.width()) * 0.5f * 64.0f;
    clipper_xy_scaling.viewport_half_height_in_1_64th_of_pixel = -static_cast<f32>(m_framebuffer_size.height()) * 0.5f * 64.0f;
    control_list.append(clipper_xy_scaling);

    ControlRecord::ClipperZScaling clipper_z_scaling {};
    clipper_z_scaling.viewport_z_scale = 0.5f;
    clipper_z_scaling.viewport_z_offset = 0.5f;
    control_list.append(clipper_z_scaling);

    ControlRecord::ClipperZMinMaxClippingPlanes clip_z_min_max_clipping_planes {};
    clip_z_min_max_clipping_planes.minimum_zw = 0.0f;
    clip_z_min_max_clipping_planes.maximum_zw = 999999999.0f;
    control_list.append(clip_z_min_max_clipping_planes);

    ControlRecord::ViewportOffset viewport_offset {};
    viewport_offset.fine_x = (m_framebuffer_size.width() / 2u) * 256u;
    viewport_offset.coarse_x = 0;
    viewport_offset.fine_y = (m_framebuffer_size.height() / 2u) * 256u;
    viewport_offset.coarse_y = 0;
    control_list.append(viewport_offset);

    // struct LibGPUBlendConfiguration {
    //     GPU::BlendEquation equation;
    //     GPU::BlendFactor src_factor;
    // };
    // struct V3DBlendConfiguration {
    //     ControlRecord::BlendMode blend_mode;
    //     ControlRecord::BlendFactor src_factor;
    //     ControlRecord::BlendFactor dst_factor;
    // };
    // auto map_blend_configuration = [](

    auto map_blend_mode = [](GPU::BlendEquation blend_equation) {
        switch (blend_equation) {
        case GPU::BlendEquation::Add:
            return ControlRecord::BlendMode::Add;
        case GPU::BlendEquation::Subtract:
            return ControlRecord::BlendMode::Sub;
        case GPU::BlendEquation::ReverseSubtract:
            return ControlRecord::BlendMode::RSub;
        case GPU::BlendEquation::Min:
            return ControlRecord::BlendMode::Min;
        case GPU::BlendEquation::Max:
            return ControlRecord::BlendMode::Max;
        }

        VERIFY_NOT_REACHED();
    };

    auto map_blend_factor = [](GPU::BlendFactor blend_factor) {
        switch (blend_factor) {
        case GPU::BlendFactor::Zero:
            return ControlRecord::BlendFactor::Zero;
        case GPU::BlendFactor::One:
            return ControlRecord::BlendFactor::One;
        case GPU::BlendFactor::SrcColor:
            return ControlRecord::BlendFactor::SrcColor;
        case GPU::BlendFactor::OneMinusSrcColor:
            return ControlRecord::BlendFactor::InvSrcColor;
        case GPU::BlendFactor::DstColor:
            return ControlRecord::BlendFactor::DstColor;
        case GPU::BlendFactor::OneMinusDstColor:
            return ControlRecord::BlendFactor::InvDstColor;
        case GPU::BlendFactor::SrcAlpha:
            return ControlRecord::BlendFactor::SrcAlpha;
        case GPU::BlendFactor::OneMinusSrcAlpha:
            return ControlRecord::BlendFactor::InvSrcAlpha;
        case GPU::BlendFactor::DstAlpha:
            return ControlRecord::BlendFactor::DstAlpha;
        case GPU::BlendFactor::OneMinusDstAlpha:
            return ControlRecord::BlendFactor::InvDstAlpha;
        case GPU::BlendFactor::ConstantColor:
            return ControlRecord::BlendFactor::ConstColor;
        case GPU::BlendFactor::OneMinusConstantColor:
            return ControlRecord::BlendFactor::InvConstColor;
        case GPU::BlendFactor::ConstantAlpha:
            return ControlRecord::BlendFactor::ConstAlpha;
        case GPU::BlendFactor::OneMinusConstantAlpha:
            return ControlRecord::BlendFactor::InvConstAlpha;
        case GPU::BlendFactor::SrcAlphaSaturate:
            return ControlRecord::BlendFactor::SrcAlphaSaturate;
        }

        VERIFY_NOT_REACHED();
    };

    // ControlRecord::ColorWriteMasks color_write_masks;
    // color_write_masks.mask = 0xff'00'00'00;
    // control_list.append(color_write_masks);

    if (m_options.enable_blending) {
        ControlRecord::BlendEnables blend_enables {};
        blend_enables.mask = m_options.enable_blending ? 0xff : 0;
        control_list.append(blend_enables);

        ControlRecord::BlendCfg blend_cfg {};
        blend_cfg.alpha_blend_mode = map_blend_mode(m_options.blend_equation_alpha);
        blend_cfg.alpha_blend_src_factor = map_blend_factor(m_options.blend_source_factor);
        blend_cfg.alpha_blend_dst_factor = map_blend_factor(m_options.blend_destination_factor);
        blend_cfg.color_blend_mode = map_blend_mode(m_options.blend_equation_alpha);
        blend_cfg.color_blend_src_factor = map_blend_factor(m_options.blend_source_factor);
        blend_cfg.color_blend_dst_factor = map_blend_factor(m_options.blend_destination_factor);
        blend_cfg.render_target_mask = 0xff;
        control_list.append(blend_cfg);

        ControlRecord::BlendConstantColor blend_constant_color {};
        blend_constant_color.red = static_cast<_Float16>(m_options.blend_color.x());
        blend_constant_color.green = static_cast<_Float16>(m_options.blend_color.y());
        blend_constant_color.blue = static_cast<_Float16>(m_options.blend_color.z());
        blend_constant_color.alpha = static_cast<_Float16>(m_options.blend_color.w());
        control_list.append(blend_constant_color);
    }

    ControlRecord::ZeroAllFlatShadeFlags zero_all_flat_shade_flags {};
    control_list.append(zero_all_flat_shade_flags);

    ControlRecord::ZeroAllNonPerspectiveFlags zero_all_nonperspective_flags {};
    control_list.append(zero_all_nonperspective_flags);

    ControlRecord::ZeroAllCentroidFlags zero_all_centroid_flags {};
    control_list.append(zero_all_centroid_flags);

    return control_list;
}

ErrorOr<ControlList> Device::generate_tile_list(u32 target_buffer_pitch, u32 target_buffer_address)
{
    ControlList control_list;

    ControlRecord::TileCoordinatesImplicit tile_coordinates_implicit {};
    control_list.append(tile_coordinates_implicit);

    if (!m_color_buffer_clear_requested_this_frame) {
        // Load the previous target buffer contents if no color buffer clear was requested.
        // Otherwise the buffer contents will get initialized with the specified clear color.
        ControlRecord::LoadTileBufferGeneral load_tile_buffer_general {};
        load_tile_buffer_general.buffer_to_load = ControlRecord::LoadTileBufferGeneral::BufferToLoad::RenderTarget0;
        load_tile_buffer_general.memory_format = ControlRecord::MemoryFormat::Raster;
        load_tile_buffer_general.flip_y = false;
        load_tile_buffer_general.decimate_mode = ControlRecord::DecimateMode::Sample0;
        load_tile_buffer_general.input_image_format = ControlRecord::OutputImageFormat::RGBA8;
        load_tile_buffer_general.force_alpha_1 = false;
        load_tile_buffer_general.channel_reverse = false;
        load_tile_buffer_general.r_b_swap = true;
        load_tile_buffer_general.height_in_ub_or_stride = target_buffer_pitch;
        load_tile_buffer_general.height = 0;
        load_tile_buffer_general.address = target_buffer_address;
        control_list.append(load_tile_buffer_general);
    }

    ControlRecord::EndOfLoads end_of_loads {};
    control_list.append(end_of_loads);

    ControlRecord::PrimListFormat prim_list_format {};
    prim_list_format.primitive_type = ControlRecord::PrimListFormat::PrimitiveType::ListTriangles;
    prim_list_format.tri_strip_or_fan = false;
    control_list.append(prim_list_format);

    ControlRecord::BranchToImplicitTileList branch_to_implicit_tile_list {};
    branch_to_implicit_tile_list.tile_list_set_number = 0;
    control_list.append(branch_to_implicit_tile_list);

    ControlRecord::StoreTileBufferGeneral store_tile_buffer_general {};
    store_tile_buffer_general.buffer_to_store = ControlRecord::StoreTileBufferGeneral::BufferToStore::RenderTarget0;
    store_tile_buffer_general.memory_format = ControlRecord::MemoryFormat::Raster;
    store_tile_buffer_general.flip_y = false;
    store_tile_buffer_general.dither_mode = ControlRecord::DitherMode::None;
    store_tile_buffer_general.decimate_mode = ControlRecord::DecimateMode::Sample0;
    store_tile_buffer_general.output_image_format = ControlRecord::OutputImageFormat::RGBA8;
    store_tile_buffer_general.clear_buffer_being_stored = false;
    store_tile_buffer_general.channel_reverse = false;
    store_tile_buffer_general.r_b_swap = true;
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

    TRY(control_list.copy_to_gpu_buffer());

    return control_list;
}

ErrorOr<Device::RenderControlList> Device::generate_render_control_list(u32 target_buffer_pitch, u32 target_buffer_address)
{
    ControlList control_list;

    ControlRecord::TileRenderingModeCfgCommon tile_rendering_mode_cfg_common {};
    static_assert(TILE_WIDTH == 64);
    static_assert(TILE_HEIGHT == 64);
    tile_rendering_mode_cfg_common.number_of_render_targets_minus_one = 1 - 1;
    tile_rendering_mode_cfg_common.image_width_pixels = m_framebuffer_size.width();
    tile_rendering_mode_cfg_common.image_height_pixels = m_framebuffer_size.height();
    tile_rendering_mode_cfg_common.multisample_mode_4x = false;
    tile_rendering_mode_cfg_common.double_buffer_in_non_ms_mode = false;
    tile_rendering_mode_cfg_common.depth_buffer_disable = false;
    tile_rendering_mode_cfg_common.early_z_test_and_update_direction = ControlRecord::TileRenderingModeCfgCommon::EarlyZTestAndUpdateDirection::LT_LE;
    tile_rendering_mode_cfg_common.early_z_disable = false;
    tile_rendering_mode_cfg_common.internal_depth_type = ControlRecord::InternalDepthType::Depth16;
    tile_rendering_mode_cfg_common.early_depth_stencil_clear = true;
    tile_rendering_mode_cfg_common.log2_tile_width = ControlRecord::TileRenderingModeCfgCommon::Log2TileWidth::Width64Pixels;
    tile_rendering_mode_cfg_common.log2_tile_height = ControlRecord::TileRenderingModeCfgCommon::Log2TileHeight::Height64Pixels;
    control_list.append(tile_rendering_mode_cfg_common);

    ControlRecord::TileRenderingModeCfgRenderTargetPart1 tile_rendering_mode_cfg_render_target_part1 {};
    tile_rendering_mode_cfg_render_target_part1.render_target_number = 0;
    tile_rendering_mode_cfg_render_target_part1.base_address = 0;
    tile_rendering_mode_cfg_render_target_part1.stride_minus_one = 32 - 1;
    tile_rendering_mode_cfg_render_target_part1.internal_bpp = 0;
    tile_rendering_mode_cfg_render_target_part1.internal_type_and_clamping = ControlRecord::RenderTargetTypeClamp::TypeClamp8;
    tile_rendering_mode_cfg_render_target_part1.clear_color_low_bits = m_clear_color;
    control_list.append(tile_rendering_mode_cfg_render_target_part1);

    ControlRecord::TileRenderingModeCfgZSClearValues tile_rendering_mode_cfg_zs_clear_values {};
    tile_rendering_mode_cfg_zs_clear_values.z_clear_value = m_clear_depth;
    tile_rendering_mode_cfg_zs_clear_values.stencil_clear_value = 0;
    control_list.append(tile_rendering_mode_cfg_zs_clear_values);

    ControlRecord::TileListInitialBlockSize tile_list_initial_block_size {};
    static_assert(TILE_ALLOC_MEMORY_INITIAL_BLOCK_SIZE == 128);
    tile_list_initial_block_size.size_of_first_block_in_chained_tile_lists = ControlRecord::TileListInitialBlockSize::SizeOfFirstBlockInChainedTileLists::Size128Bytes;
    tile_list_initial_block_size.use_auto_chained_tile_lists = true;
    control_list.append(tile_list_initial_block_size);

    ControlRecord::MulticoreRenderingTileListSetBase multicore_rendering_tile_list_set_base {};
    multicore_rendering_tile_list_set_base.tile_list_set_number = 0;
    multicore_rendering_tile_list_set_base.address = m_tile_alloc_memory_buffer->gpu_virtual_address() >> 6u;
    control_list.append(multicore_rendering_tile_list_set_base);

    ControlRecord::MulticoreRenderingSupertileCfg multicore_rendering_supertile_cfg {};
    multicore_rendering_supertile_cfg.supertile_width_in_tiles_minus_one = 1 - 1;
    multicore_rendering_supertile_cfg.supertile_height_in_tiles_minus_one = 1 - 1;
    multicore_rendering_supertile_cfg.total_frame_width_in_supertiles = ceil_div(m_framebuffer_size.width(), TILE_WIDTH);
    multicore_rendering_supertile_cfg.total_frame_height_in_supertiles = ceil_div(m_framebuffer_size.height(), TILE_HEIGHT);
    multicore_rendering_supertile_cfg.total_frame_width_in_tiles = ceil_div(m_framebuffer_size.width(), TILE_WIDTH);
    multicore_rendering_supertile_cfg.total_frame_height_in_tiles = ceil_div(m_framebuffer_size.height(), TILE_HEIGHT);
    multicore_rendering_supertile_cfg.multicore_enable = false;
    multicore_rendering_supertile_cfg.supertile_raster_order = false;
    multicore_rendering_supertile_cfg.number_of_bin_tile_lists_minus_one = 1 - 1;
    control_list.append(multicore_rendering_supertile_cfg);

    ControlRecord::TileCoordinates tile_coordinates {};
    tile_coordinates.tile_column_number = 0;
    tile_coordinates.tile_row_number = 0;
    control_list.append(tile_coordinates);

    ControlRecord::EndOfLoads end_loads {};
    control_list.append(end_loads);

    ControlRecord::StoreTileBufferGeneral store_tile_buffer_general {};
    store_tile_buffer_general.buffer_to_store = ControlRecord::StoreTileBufferGeneral::BufferToStore::None;
    store_tile_buffer_general.memory_format = ControlRecord::MemoryFormat::Raster;
    store_tile_buffer_general.flip_y = false;
    store_tile_buffer_general.dither_mode = ControlRecord::DitherMode::None;
    store_tile_buffer_general.decimate_mode = ControlRecord::DecimateMode::Sample0;
    store_tile_buffer_general.output_image_format = ControlRecord::OutputImageFormat::RGBA8;
    store_tile_buffer_general.clear_buffer_being_stored = false;
    store_tile_buffer_general.channel_reverse = false;
    store_tile_buffer_general.r_b_swap = false;
    store_tile_buffer_general.height_in_ub_or_stride = 0;
    store_tile_buffer_general.height = 0;
    store_tile_buffer_general.address = 0;
    control_list.append(store_tile_buffer_general);

    ControlRecord::ClearRenderTargets clear_render_targets {};
    control_list.append(clear_render_targets);

    ControlRecord::EndOfTileMarker end_of_tile_marker {};
    control_list.append(end_of_tile_marker);

    // FIXME: Mesa seems to emit a second store here?

    ControlRecord::FlushVCDCache flush_vcd_cache {};
    control_list.append(flush_vcd_cache);

    auto tile_list = TRY(generate_tile_list(target_buffer_pitch, target_buffer_address));

    ControlRecord::StartAddressOfGenericTileList start_address_of_generic_tile_list {};
    start_address_of_generic_tile_list.start = tile_list.buffer()->gpu_virtual_address();
    start_address_of_generic_tile_list.end = tile_list.buffer()->gpu_virtual_address() + tile_list.data().size();
    control_list.append(start_address_of_generic_tile_list);

    for (int row_number_in_supertiles = 0; row_number_in_supertiles < ceil_div(m_framebuffer_size.height(), TILE_HEIGHT); row_number_in_supertiles++) {
        for (int column_number_in_supertiles = 0; column_number_in_supertiles < ceil_div(m_framebuffer_size.width(), TILE_WIDTH); column_number_in_supertiles++) {
            ControlRecord::SupertileCoordinates supertile_coordinates {};
            supertile_coordinates.column_number_in_supertiles = column_number_in_supertiles;
            supertile_coordinates.row_number_in_supertiles = row_number_in_supertiles;
            control_list.append(supertile_coordinates);
        }
    }

    ControlRecord::EndOfRendering end_of_rendering {};
    control_list.append(end_of_rendering);

    TRY(control_list.copy_to_gpu_buffer());

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
    // FIXME: Don't hardcode this path.
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

    m_framebuffer = TRY(Buffer::create(align_up_to(m_framebuffer_size.area() * sizeof(u32), V3D_PAGE_SIZE), "LibV3DGPU: Framebuffer"sv));
    m_framebuffer_data = m_framebuffer->data();

    m_binner_control_list = TRY(generate_initial_binner_control_list());

    auto tile_count = ceil_div(m_framebuffer_size.width(), TILE_WIDTH) * ceil_div(m_framebuffer_size.height(), TILE_HEIGHT);

    auto tile_alloc_memory_size = align_up_to(tile_count * TILE_ALLOC_MEMORY_INITIAL_BLOCK_SIZE, V3D_PAGE_SIZE);
    // Add some extra memory to avoid having the kernel allocate overspill memory.
    // FIXME: There is probably a better way to calculate this.
    tile_alloc_memory_size += 1 * MiB;
    m_tile_alloc_memory_buffer = TRY(Buffer::create(tile_alloc_memory_size, "LibV3DGPU: Tile Alloc Memory"sv));

    auto tile_state_data_array_memory_size = align_up_to(tile_count * TILE_STATE_DATA_ARRAY_ELEMENT_SIZE, V3D_PAGE_SIZE);
    m_tile_state_data_array_buffer = TRY(Buffer::create(tile_state_data_array_memory_size, "LibV3DGPU: Tile State Data Array"sv));

    m_placeholder_sampler_configuration.bound_image = create_image(GPU::PixelFormat::RGBA, 1, 1, 1, 1);

    GPU::ImageDataLayout placeholder_layout = {
        .pixel_type = { GPU::PixelFormat::RGBA, GPU::PixelComponentBits::AllBits, GPU::PixelDataType::Byte, GPU::ComponentsOrder::Normal },
        .packing = { 1, 1, 4, GPU::ComponentBytesOrder::Normal },
        .dimensions = {
            .width = 1,
            .height = 1,
            .depth = 1,
        },
        .selection = {
            .width = 1,
            .height = 1,
            .depth = 1,
        },
    };

    u32 placeholder_texture[1] = { 0xff'ff'ff'ff };
    m_placeholder_sampler_configuration.bound_image->write_texels(0, { 0, 0, 0 }, &placeholder_texture, placeholder_layout);

    return {};
}

GPU::DeviceInfo Device::info() const
{
    return {
        .vendor_name = "SerenityOS",
        .device_name = "VideoCore VII 3D",
        .num_texture_units = 1,
        .num_lights = 8,
        .max_clip_planes = 0,
        .max_texture_size = 16384,
        .max_texture_lod_bias = 0.f,
        .stencil_bits = 0,
        .supports_npot_textures = true,
        .supports_texture_clamp_to_edge = false,
        .supports_texture_env_add = false,
    };
}

void Device::draw_primitives(GPU::PrimitiveType primitive_type, Vector<GPU::Vertex>& vertices)
{
    if (vertices.size() == 0)
        return;

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
            .w = vertex.position.w(),
            .r = vertex.color.x(),
            .g = vertex.color.y(),
            .b = vertex.color.z(),
            .u = vertex.tex_coords[0].x(),
            .v = vertex.tex_coords[0].y(),
        };
    };

    if (primitive_type == GPU::PrimitiveType::Quads) {
        if (vertices.size() < 4)
            return;

        size_t quad_count = vertices.size() / 4;

        vertex_array.ensure_capacity(quad_count * 6);

        for (size_t i = 0; i < vertices.size() - 3; i += 4) {
            vertex_array.append(convert_vertex(vertices[i + 0]));
            vertex_array.append(convert_vertex(vertices[i + 1]));
            vertex_array.append(convert_vertex(vertices[i + 2]));

            vertex_array.append(convert_vertex(vertices[i + 2]));
            vertex_array.append(convert_vertex(vertices[i + 3]));
            vertex_array.append(convert_vertex(vertices[i + 0]));
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
    cfg_bits.enable_depth_offset = m_options.depth_offset_enabled;
    cfg_bits.line_rasterization = 0;
    cfg_bits.depth_bounds_test_enable = 0;
    cfg_bits.rasterizer_oversample_mode = 0;
    cfg_bits.z_clamp_mode = 0;
    cfg_bits.direct3d_wireframe_triangles_mode = 0;
    cfg_bits.depth_test_function = determine_depth_test_function();
    cfg_bits.z_updates_enable = m_options.enable_depth_test && m_options.enable_depth_write;
    cfg_bits.stencil_enable = 0;
    cfg_bits.blend_enable = m_options.enable_blending;
    cfg_bits.direct3d_point_fill_mode = 0;
    cfg_bits.direct3d_provoking_vertex = 0;
    cfg_bits.z_clipping_mode = ControlRecord::ZClipMode::None;
    m_binner_control_list.append(cfg_bits);

    if (m_options.depth_offset_enabled) {
        ControlRecord::DepthOffset depth_offset;
        depth_offset.depth_offset_factor = bit_cast<u32>(m_options.depth_offset_factor) >> 16;
        depth_offset.depth_offset_units = bit_cast<u32>(m_options.depth_offset_constant) >> 16;
        depth_offset.limit = 0.0f;
        m_binner_control_list.append(depth_offset);
    }

    ControlRecord::GLShaderState gl_shader_state {};
    gl_shader_state.number_of_attribute_arrays = 3;
    gl_shader_state.address = shader_state_record.control_list.buffer()->gpu_virtual_address() >> 5;
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
    m_color_buffer_clear_requested_this_frame = true;

    auto clamped = color.clamped(0.0f, 1.0f);
    auto r = static_cast<u8>(clamped.x() * 255u);
    auto g = static_cast<u8>(clamped.y() * 255u);
    auto b = static_cast<u8>(clamped.z() * 255u);
    auto a = static_cast<u8>(clamped.w() * 255u);
    m_clear_color = a << 24u | b << 16u | g << 8u | r;
}

void Device::clear_depth(GPU::DepthType depth)
{
    m_clear_depth = clamp(depth, 0.0f, 1.0f);
}

void Device::clear_stencil(GPU::StencilType)
{
    dbgln("V3DGPU::Device::clear_stencil(): unimplemented");
}

void Device::blit_from_color_buffer(Gfx::Bitmap& target)
{
    ControlRecord::Flush flush {};
    m_binner_control_list.append(flush);

    m_binner_control_list.copy_to_gpu_buffer().release_value_but_fixme_should_propagate_errors();

    auto new_render_control_list_state = render_control_list_state_needed_for_current_frame();
    if (m_render_control_list_state != new_render_control_list_state) {
        // Some parameter(s) changed that require regenerating the render control list.
        m_render_control_list = generate_render_control_list(m_framebuffer_size.width() * sizeof(u32), m_framebuffer->gpu_virtual_address()).release_value_but_fixme_should_propagate_errors();

        m_render_control_list_state = new_render_control_list_state;
    }

    V3DJob kernel_job = {
        .tile_state_data_array_address = m_tile_state_data_array_buffer->gpu_virtual_address(),
        .tile_allocation_memory_address = m_tile_alloc_memory_buffer->gpu_virtual_address(),
        .tile_allocation_memory_size = m_tile_alloc_memory_buffer->size(),

        .binning_control_list_address = m_binner_control_list.buffer()->gpu_virtual_address(),
        .binning_control_list_size = static_cast<u32>(m_binner_control_list.data().size()),

        .rendering_control_list_address = m_render_control_list.control_list.buffer()->gpu_virtual_address(),
        .rendering_control_list_size = static_cast<u32>(m_render_control_list.control_list.data().size()),
    };

    auto submit_job_result = Core::System::ioctl(g_v3d_fd, V3D_SUBMIT_JOB, &kernel_job);
    if (submit_job_result.is_error()) {
        dbgln("LibV3DGPU: Job submission failed: {}", submit_job_result.error());

        dbgln("Framebuffer: {}", m_framebuffer);

        dbgln("Tile state data array: {}", m_tile_state_data_array_buffer);
        dbgln("Tile alloc memory: {}", m_tile_alloc_memory_buffer);

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

    // FIXME: Add support for other pitches.
    VERIFY(target.pitch() == m_framebuffer_size.width() * sizeof(u32));
    VERIFY(target.data_size() == m_framebuffer_size.area() * sizeof(u32));
    VERIFY(target.size() == m_framebuffer_size);

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

    relatively_fast_copy(target.scanline_u8(0), m_framebuffer_data.data(), target.data_size());
#else
    memcpy(target.scanline_u8(0), m_framebuffer_data.data(), target.data_size());
#endif

    m_binner_control_list = generate_initial_binner_control_list().release_value_but_fixme_should_propagate_errors();
    m_shader_state_records.clear();

    m_color_buffer_clear_requested_this_frame = false;
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
    // dbgln("V3DGPU::Device::set_light_model_params(): unimplemented");
}

GPU::RasterizerOptions Device::options() const
{
    return m_options;
}

GPU::LightModelParameters Device::light_model() const
{
    // dbgln("V3DGPU::Device::light_model(): unimplemented");
    return {};
}

NonnullRefPtr<GPU::Image> Device::create_image(GPU::PixelFormat const& pixel_format, u32 width, u32 height, u32 depth, u32 max_levels)
{
    VERIFY(width > 0);
    VERIFY(height > 0);
    VERIFY(depth > 0);
    VERIFY(max_levels > 0);

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

void Device::set_sampler_config(unsigned sampler, GPU::SamplerConfig const& config)
{
    VERIFY(config.bound_image.is_null() || config.bound_image->ownership_token() == this);

    m_sampler_configuration[sampler] = config;

    // auto& image = *reinterpret_cast<V3DGPU::Image*>(m_sampler_configuration[0].bound_image.ptr());

    // dbgln("Image:");
    // dump(image.data());

    // dbgln("texture @ {:#x}", texture_buffer.gpu_virtual_address());

    //
    // m_samplers_need_texture_staging = any_of(m_samplers, [](auto const& sampler) {
    //     auto const& fixed_function_env = sampler.config().fixed_function_texture_environment;
    //     if (fixed_function_env.env_mode != GPU::TextureEnvMode::Combine)
    //         return false;
    //     return any_of(fixed_function_env.alpha_source, [](auto texture_source) { return texture_source == GPU::TextureSource::TextureStage; })
    //         || any_of(fixed_function_env.rgb_source, [](auto texture_source) { return texture_source == GPU::TextureSource::TextureStage; });
    // });
}

void Device::set_light_state(unsigned, GPU::Light const&)
{
    // dbgln("V3DGPU::Device::set_light_state(): unimplemented");
}

void Device::set_material_state(GPU::Face, GPU::Material const&)
{
    // dbgln("V3DGPU::Device::set_material_state(): unimplemented");
}

void Device::set_stencil_configuration(GPU::Face, GPU::StencilConfiguration const&)
{
    dbgln("V3DGPU::Device::set_stencil_configuration(): unimplemented");
}

void Device::set_texture_unit_configuration(GPU::TextureUnitIndex index, GPU::TextureUnitConfiguration const& configuration)
{
    m_texture_unit_configuration[index] = configuration;
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
