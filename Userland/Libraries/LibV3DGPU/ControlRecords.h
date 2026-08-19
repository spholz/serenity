/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

namespace ControlRecord {

// Reference for control records: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/broadcom/cle/v3d_packet.xml

// <enum name="Compare Function" prefix="V3D_COMPARE_FUNC">
enum class CompareFunction : u64 {
    Never = 0,
    Less = 1,
    Equal = 2,
    LEqual = 3,
    Greater = 4,
    NotEqual = 5,
    GEqual = 6,
    Always = 7,
};

// <enum name="Primitive" prefix="V3D_PRIM">
enum class Primitive : u32 {
    Points = 0,
    Lines = 1,
    LineLoop = 2,
    LineStrip = 3,
    Triangles = 4,
    TriangleStrip = 5,
    TriangleFan = 6,
};

// <packet code="4" name="Flush"/>
struct [[gnu::packed]] Flush {
    u8 opcode = 4;
};
static_assert(AssertSize<Flush, 1>());

// <packet code="6" name="Start Tile Binning"/>
struct [[gnu::packed]] StartTileBinning {
    u8 opcode = 6;
};
static_assert(AssertSize<StartTileBinning, 1>());

// <packet code="13" shortname="end_render" name="End of rendering"/>
struct [[gnu::packed]] EndOfRendering {
    u8 opcode = 13;
};
static_assert(AssertSize<EndOfRendering, 1>());

// <packet code="18" shortname="return" name="Return from sub-list"/>
struct [[gnu::packed]] ReturnFromSubList {
    u8 opcode = 18;
};
static_assert(AssertSize<ReturnFromSubList, 1>());

// <packet code="19" shortname="clear_vcd_cache" name="Flush VCD cache"/>
struct [[gnu::packed]] FlushVCDCache {
    u8 opcode = 19;
};
static_assert(AssertSize<FlushVCDCache, 1>());

// <packet code="20" shortname="generic_tile_list" name="Start Address of Generic Tile List">
struct [[gnu::packed]] StartAddressOfGenericTileList {
    u8 opcode = 20;
    u32 start : 32;
    u32 end : 32;
};
static_assert(AssertSize<StartAddressOfGenericTileList, 1 + 8>());

// <packet code="21" shortname="branch_implicit_tile" name="Branch to Implicit Tile List">
struct [[gnu::packed]] BranchToImplicitTileList {
    u8 opcode = 21;
    u8 tile_list_set_number : 8;
};
static_assert(AssertSize<BranchToImplicitTileList, 1 + 1>());

// <packet code="23" shortname="supertile_coords" name="Supertile Coordinates">
struct [[gnu::packed]] SupertileCoordinates {
    u8 opcode = 23;
    u16 column_number_in_supertiles : 8;
    u16 row_number_in_supertiles : 8;
};
static_assert(AssertSize<SupertileCoordinates, 1 + 2>());

// <packet code="25" shortname="clear_rt" name="Clear Render Targets" cl="R" min_ver="71"/>
struct [[gnu::packed]] ClearRenderTargets {
    u8 opcode = 25;
};
static_assert(AssertSize<ClearRenderTargets, 1>());

// <packet code="26" shortname="end_loads" name="End of Loads" cl="R"/>
struct [[gnu::packed]] EndOfLoads {
    u8 opcode = 26;
};
static_assert(AssertSize<EndOfLoads, 1>());

// <packet code="27" shortname="end_tile" name="End of Tile Marker" cl="R"/>
struct [[gnu::packed]] EndOfTileMarker {
    u8 opcode = 27;
};
static_assert(AssertSize<EndOfTileMarker, 1>());

// <packet code="29" shortname="store" name="Store Tile Buffer General" cl="R">
struct [[gnu::packed]] StoreTileBufferGeneral {
    u8 opcode = 29;
    u64 buffer_to_store : 4;
    u64 memory_format : 3;
    u64 flip_y : 1;
    u64 dither_mode : 2;
    u64 decimate_mode : 2;
    u64 output_image_format : 6;
    u64 clear_buffer_being_stored : 1;
    u64 channel_reverse : 1;
    u64 r_b_swap : 1;
    u64 _reserved0 : 7;
    u64 height_in_ub_or_stride : 20;
    u64 height : 16;
    u64 address : 32;
};
static_assert(AssertSize<StoreTileBufferGeneral, 1 + 12>());

// <packet code="36" name="Vertex Array Prims" cl="B">
struct [[gnu::packed]] VertexArrayPrims {
    u8 opcode = 36;
    Primitive mode : 8;
    u32 length : 32;
    u32 index_of_first_vertex : 32;
};
static_assert(AssertSize<VertexArrayPrims, 1 + 9>());

// <packet code="54" name="Set InstanceID" cl="B">
struct [[gnu::packed]] SetInstanceID {
    u8 opcode = 54;
    u32 instance_id : 32;
};
static_assert(AssertSize<SetInstanceID, 1 + 4>());

// <packet code="56" name="Prim List Format">
struct [[gnu::packed]] PrimListFormat {
    u8 opcode = 56;
    u8 primitive_type : 6;
    u8 _reserved0 : 1;
    u8 tri_strip_or_fan : 1;
};
static_assert(AssertSize<PrimListFormat, 1 + 1>());

// <packet code="64" shortname="gl_shader" name="GL Shader State">
struct [[gnu::packed]] GLShaderState {
    u8 opcode = 64;
    u32 number_of_attribute_arrays : 5;
    u32 address : 27;
};
static_assert(AssertSize<GLShaderState, 1 + 4>());

// <packet code="71" name="VCM Cache Size">
struct [[gnu::packed]] VCMCacheSize {
    u8 opcode = 71;
    u8 number_of_16_vertex_batches_for_binning : 4;
    u8 number_of_16_vertex_batches_for_rendering : 4;
};
static_assert(AssertSize<VCMCacheSize, 1 + 1>());

// <packet code="74" name="Transform Feedback Specs">
struct [[gnu::packed]] TransformFeedbackSpecs {
    u8 opcode = 74;
    u8 number_of_16bit_output_data_specs_following : 5;
    u8 _reserved0 : 1;
    u8 enable : 1;
};
static_assert(AssertSize<TransformFeedbackSpecs, 1 + 1>());

// <packet code="86" shortname="blend_ccolor" name="Blend Constant Color">
struct [[gnu::packed]] BlendConstantColor {
    u8 opcode = 86;
    u64 red : 16;   // float?
    u64 green : 16; // float?
    u64 blue : 16;  // float?
    u64 alpha : 16; // float?
};
static_assert(AssertSize<BlendConstantColor, 1 + 8>());

// <packet code="87" shortname="color_wmasks" name="Color Write Masks">
struct [[gnu::packed]] ColorWriteMasks {
    u8 opcode = 87;
    u32 mask : 32;
};
static_assert(AssertSize<ColorWriteMasks, 1 + 4>());

// <packet code="88" name="Zero All Centroid Flags" />
struct [[gnu::packed]] ZeroAllCentroidFlags {
    u8 opcode = 88;
};

// <packet code="91" name="Sample State">
struct [[gnu::packed]] SampleState {
    u8 opcode = 91;
    u32 mask : 4;
    u32 _reserved0 : 12;
    u32 coverage : 16; // f187
};
static_assert(AssertSize<SampleState, 1 + 4>());

// <packet code="92" shortname="occlusion_query_counter_enable" name="Occlusion Query Counter">
struct [[gnu::packed]] OcclusionQueryCounter {
    u8 opcode = 92;
    u32 address : 32;
};
static_assert(AssertSize<OcclusionQueryCounter, 1 + 4>());

// <packet code="96" name="Cfg Bits" min_ver="71">
struct [[gnu::packed]] CfgBits {
    u8 opcode = 96;
    u64 enable_forward_facing_primitive : 1;
    u64 enable_reverse_facing_primitive : 1;
    u64 clockwise_primitives : 1;
    u64 enable_depth_offset : 1;
    u64 line_rasterization : 1;
    u64 depth_bounds_test_enable : 1;
    u64 rasterizer_oversample_mode : 2;
    u64 _reserved0 : 2;
    u64 z_clamp_mode : 1;
    u64 direct3d_wireframe_triangles_mode : 1;
    CompareFunction depth_test_function : 3;
    u64 z_updates_enable : 1;
    u64 _reserved1 : 2;
    u64 stencil_enable : 1;
    u64 blend_enable : 1;
    u64 direct3d_point_fill_mode : 1;
    u64 direct3d_provoking_vertex : 1;
    u64 z_clipping_mode : 2;
};
static_assert(AssertSize<CfgBits, 1 + 3>());

// <packet code="97" shortname="zero_all_flatshade_flags" name="Zero All Flat Shade Flags"/>
struct [[gnu::packed]] ZeroAllFlatShadeFlags {
    u8 opcode = 97;
};
static_assert(AssertSize<ZeroAllFlatShadeFlags, 1>());

// <packet code="99" shortname="zero_all_noperspective_flags" name="Zero All Non-perspective Flags" />
struct [[gnu::packed]] ZeroAllNonPerspectiveFlags {
    u8 opcode = 99;
};
static_assert(AssertSize<ZeroAllNonPerspectiveFlags, 1>());

// <packet code="104" name="Point size">
struct [[gnu::packed]] PointSize {
    u8 opcode = 104;
    u32 point_size : 32; // float
};
static_assert(AssertSize<PointSize, 1 + 4>());

// <packet code="105" name="Line width">
struct [[gnu::packed]] LineWidth {
    u8 opcode = 105;
    u32 line_width : 32; // float
};
static_assert(AssertSize<LineWidth, 1 + 4>());

// <packet shortname="clip" name="clip_window" code="107">
struct [[gnu::packed]] ClipWindow {
    u8 opcode = 107;
    u64 clip_window_left_pixel_coordinate : 16;
    u64 clip_window_bottom_pixel_coordinate : 16;
    u64 clip_window_width_in_pixels : 16;
    u64 clip_window_height_in_pixels : 16;
};
static_assert(AssertSize<ClipWindow, 1 + 8>());

// <packet name="Viewport Offset" code="108">
struct [[gnu::packed]] ViewportOffset {
    u8 opcode = 108;
    u64 fine_x : 22; // unsigned 14.8 fixed point
    i64 coarse_x : 10;
    u64 fine_y : 22; // unsigned 14.8 fixed point
    i64 coarse_y : 10;
};
static_assert(AssertSize<ViewportOffset, 1 + 8>());

// <packet shortname="clipz" name="Clipper Z min/max clipping planes" code="109">
struct [[gnu::packed]] ClipperZMinMaxClippingPlanes {
    u8 opcode = 109;
    u32 minimum_zw : 32; // float
    u32 maximum_zw : 32; // float
};
static_assert(AssertSize<ClipperZMinMaxClippingPlanes, 1 + 8>());

// <packet shortname="clipper_xy" name="Clipper XY Scaling" code="110" cl="B" min_ver="71">
struct [[gnu::packed]] ClipperXYScaling {
    u8 opcode = 110;
    u32 viewport_half_width_in_1_64th_of_pixel : 32;  // float
    u32 viewport_half_height_in_1_64th_of_pixel : 32; // float
};
static_assert(AssertSize<ClipperXYScaling, 1 + 8>());

// <packet shortname="clipper_z" name="Clipper Z Scale and Offset" code="111" cl="B">
struct [[gnu::packed]] ClipperZScaling {
    u8 opcode = 111;
    u32 viewport_z_scale : 32;  // float
    u32 viewport_z_offset : 32; // float
};
static_assert(AssertSize<ClipperZScaling, 1 + 8>());

// <packet name="Number of Layers" code="119">
struct [[gnu::packed]] NumberOfLayers {
    u8 opcode = 119;
    u8 number_of_layers_minus_one : 8;
};
static_assert(AssertSize<NumberOfLayers, 1 + 1>());

// <packet code="120" name="Tile Binning Mode Cfg" min_ver="71">
struct [[gnu::packed]] TileBinningModeCfg {
    u8 opcode = 120;
    u64 _reserved0 : 2;
    u64 tile_allocation_initial_block_size : 2;
    u64 tile_allocation_block_size : 2;
    u64 _reserved1 : 2;
    u64 log2_tile_width : 3;
    u64 log2_tile_height : 3;
    u64 _reserved2 : 18;
    u64 width_in_pixels_minus_one : 16;
    u64 height_in_pixels_minus_one : 16;
};
static_assert(AssertSize<TileBinningModeCfg, 9>());

// <packet code="121" name="Tile Rendering Mode Cfg (Common)" cl="R" min_ver="71">
struct [[gnu::packed]] TileRenderingModeCfgCommon {
    u8 opcode = 121;
    u64 sub_id : 3 = 0;
    u64 _reserved0 : 1;
    u64 number_of_render_targets_minus_one : 4;
    u64 image_width_pixels : 16;
    u64 image_height_pixels : 16;
    u64 _reserved1 : 2;
    u64 multisample_mode_4x : 1;
    u64 double_buffer_in_non_ms_mode : 1;
    u64 depth_buffer_disable : 1;
    u64 early_z_test_and_update_direction : 1;
    u64 early_z_disable : 1;
    u64 internal_depth_type : 4;
    u64 early_depth_stencil_clear : 1;
    u64 log2_tile_width : 3;
    u64 log2_tile_height : 3;
    u64 pad : 6;
};
static_assert(AssertSize<TileRenderingModeCfgCommon, 1 + 8>());

// <packet code="121" name="Tile Rendering Mode Cfg (ZS Clear Values)" cl="R" min_ver="71">
struct [[gnu::packed]] TileRenderingModeCfgZSClearValues {
    u8 opcode = 121;
    u64 sub_id : 4 = 1; // Is this an error in v3d_packet.xml? the sub-id is 3 bits in other packets.
    u8 _reserved0 : 4;
    u64 stencil_clear_value : 8;
    u64 z_clear_value : 32; // float
    u64 unused : 16;
};
static_assert(AssertSize<TileRenderingModeCfgZSClearValues, 1 + 8>());

// <packet code="121" name="Tile Rendering Mode Cfg (Render Target Part1)" cl="R" min_ver="71">
struct [[gnu::packed]] TileRenderingModeCfgRenderTargetPart1 {
    u8 opcode = 121;
    u64 sub_id : 3 = 2;
    u64 render_target_number : 3;
    u64 _reserved0 : 1;
    u64 base_address : 11;
    u64 stride_minus_one : 7;
    u64 internal_bpp : 2;
    u64 internal_type_and_clamping : 5;
    u64 clear_color_low_bits : 32;
};
static_assert(AssertSize<TileRenderingModeCfgRenderTargetPart1, 1 + 8>());

// <packet code="122" name="Multicore Rendering Supertile Cfg" cl="R">
struct [[gnu::packed]] MulticoreRenderingSupertileCfg {
    u8 opcode = 122;
    u64 supertile_width_in_tiles_minus_one : 8;
    u64 supertile_height_in_tiles_minus_one : 8;
    u64 total_frame_width_in_supertiles : 8;
    u64 total_frame_height_in_supertiles : 8;
    u64 total_frame_width_in_tiles : 12;
    u64 total_frame_height_in_tiles : 12;
    u64 multicore_enable : 1;
    u64 _reserved0 : 3;
    u64 supertile_raster_order : 1;
    u64 number_of_bin_tile_lists_minus_one : 3;
};
static_assert(AssertSize<MulticoreRenderingSupertileCfg, 1 + 8>());

// <packet code="123" shortname="multicore_rendering_tile_list_base" name="Multicore Rendering Tile List Set Base" cl="R">
struct [[gnu::packed]] MulticoreRenderingTileListSetBase {
    u8 opcode = 123;
    u32 tile_list_set_number : 4;
    u32 _reserved0 : 2;
    u32 address : 26;
};
static_assert(AssertSize<MulticoreRenderingTileListSetBase, 1 + 4>());

// <packet code="124" shortname="tile_coords" name="Tile Coordinates">
struct [[gnu::packed]] TileCoordinates {
    u8 opcode = 124;
    u32 tile_column_number : 12;
    u32 tile_row_number : 12;
};
static_assert(AssertSize<TileCoordinates, 1 + 3>());

// <packet code="125" shortname="implicit_tile_coords" name="Tile Coordinates Implicit"/>
struct [[gnu::packed]] ImplicitTileCoordinates {
    u8 opcode = 125;
};
static_assert(AssertSize<ImplicitTileCoordinates, 1>());

// <packet code="126" name="Tile List Initial Block Size">
struct [[gnu::packed]] TileListInitialBlockSize {
    u8 opcode = 126;
    u8 size_of_first_block_in_chained_tile_lists : 2;
    u8 use_auto_chained_tile_lists : 1;
    u8 _reserved0 : 5;
};
static_assert(AssertSize<TileListInitialBlockSize, 1 + 1>());

// XXX: These aren't really control lists

// <struct name="GL Shader State Record" min_ver="71">
struct [[gnu::packed]] GLShaderStateRecord {
    u64 point_size_in_shaded_vertex_data : 1;
    u64 enable_clipping : 1;
    u64 vertex_id_read_by_coordinate_shader : 1;
    u64 instance_id_read_by_vertex_shader : 1;
    u64 base_instance_id_read_by_coordinate_shader : 1;
    u64 vertex_id_read_by_vertex_shader : 1;
    u64 instance_id_read_by_coordinate_shader : 1;
    u64 base_instance_id_read_by_vertex_shader : 1;
    u64 fragment_shader_does_z_writes : 1;
    u64 turn_off_early_z_test : 1;
    u64 _reserved0 : 2;
    u64 fragment_shader_uses_real_pixel_centre_w_in_addition_to_centroid_w2 : 1;
    u64 enable_sample_rate_shading : 1;
    u64 any_shader_reads_hardware_written_primitive_id : 1;
    u64 insert_primitive_id_as_first_varying_to_fragment_shader : 1;
    u64 turn_off_scoreboard : 1;
    u64 do_scoreboard_wait_on_first_thread_switch : 1;
    u64 disable_implicit_point_line_varyings : 1;
    u64 no_prim_pack : 1;
    u64 never_defer_fep_depth_writes : 1;
    u64 _reserved1 : 3;
    u64 number_of_varyings_in_fragment_shader : 8;
    u64 coordinate_shader_output_vpm_segment_size : 4;
    u64 min_coord_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size : 4;
    u64 coordinate_shader_input_vpm_segment_size : 4;
    u64 min_coord_shader_input_segments_required_in_play_minus_one : 4;
    u64 vertex_shader_output_vpm_segment_size : 4;
    u64 min_vertex_shader_output_segments_required_in_play_in_addition_to_vcm_cache_size : 4;
    u64 vertex_shader_input_vpm_segment_size : 4;
    u64 min_vertex_shader_input_segments_required_in_play_minus_one : 4;
    u64 fragment_shader_4_way_threadable : 1;
    u64 fragment_shader_start_in_final_thread_section : 1;
    u64 fragment_shader_propagate_nans : 1;
    u64 fragment_shader_code_address : 29;
    u64 fragment_shader_uniforms_address : 32;
    u64 vertex_shader_4_way_threadable : 1;
    u64 vertex_shader_start_in_final_thread_section : 1;
    u64 vertex_shader_propagate_nans : 1;
    u64 vertex_shader_code_address : 29;
    u64 vertex_shader_uniforms_address : 32;
    u64 coordinate_shader_4_way_threadable : 1;
    u64 coordinate_shader_start_in_final_thread_section : 1;
    u64 coordinate_shader_propagate_nans : 1;
    u64 coordinate_shader_code_address : 29;
    u64 coordinate_shader_uniforms_address : 32;
};
static_assert(AssertSize<GLShaderStateRecord, 32>());

// <struct name="GL Shader State Attribute Record">
struct [[gnu::packed]] GLShaderStateAttributeRecord {
    u32 address : 32;
    u64 vec_size : 2;
    u64 type : 3;
    u64 signed_int_type : 1;
    u64 normalized_int_type : 1;
    u64 read_as_int_uint : 1;
    u64 number_of_values_read_by_coordinate_shader : 4;
    u64 number_of_values_read_by_vertex_shader : 4;
    u64 instance_divisor : 16;
    u64 stride : 32;
    u64 maximum_index : 32;
};
static_assert(AssertSize<GLShaderStateAttributeRecord, 16>());

}
