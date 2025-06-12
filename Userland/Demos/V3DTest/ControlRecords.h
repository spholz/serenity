/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StdLibExtraDetails.h>
#include <AK/Types.h>

#ifdef KERNEL
namespace Kernel::RPi::V3D::ControlRecord {
#else
namespace ControlRecord {
#endif

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
//   <field name="start" size="32" start="0" type="address"/>
//   <field name="end" size="32" start="32" type="address"/>
// </packet>
struct [[gnu::packed]] StartAddressOfGenericTileList {
    u8 opcode = 20;
    u32 start : 32;
    u32 end : 32;
};
static_assert(AssertSize<StartAddressOfGenericTileList, 1 + 8>());

// <packet code="21" shortname="branch_implicit_tile" name="Branch to Implicit Tile List">
//   <field name="tile list set number" size="8" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] BranchToImplicitTileList {
    u8 opcode = 21;
    u8 tile_list_set_number : 8;
};
static_assert(AssertSize<BranchToImplicitTileList, 1 + 1>());

// <packet code="23" shortname="supertile_coords" name="Supertile Coordinates">
//   <field name="row number in supertiles" size="8" start="8" type="uint"/>
//   <field name="column number in supertiles" size="8" start="0" type="uint"/>
// </packet>
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
//   <field name="Address" size="32" start="64" type="address"/>
//
//   <!-- used for y flip -->
//   <field name="Height" size="16" start="48" type="uint"/>
//
//   <!-- height in ub for UIF, byte stride for raster -->
//   <field name="Height in UB or Stride" size="20" start="28" type="uint"/>
//
//   <field name="R/B swap" size="1" start="20" type="bool"/>
//   <field name="Channel Reverse" size="1" start="19" type="bool"/>
//   <field name="Clear buffer being stored" size="1" start="18" type="bool"/>
//   <field name="Output Image Format" size="6" start="12" type="Output Image Format"/>
//
//   <field name="Decimate mode" size="2" start="10" type="Decimate Mode"/>
//
//   <field name="Dither Mode" size="2" start="8" type="Dither Mode"/>
//
//   <field name="Flip Y" size="1" start="7" type="bool"/>
//
//   <field name="Memory Format" size="3" start="4" type="Memory Format"/>
//   <field name="Buffer to Store" size="4" start="0" type="uint">
//     <value name="Render target 0" value="0"/>
//     <value name="Render target 1" value="1"/>
//     <value name="Render target 2" value="2"/>
//     <value name="Render target 3" value="3"/>
//     <value name="Render target 4" value="4" min_ver="71"/>
//     <value name="Render target 5" value="5" min_ver="71"/>
//     <value name="Render target 6" value="6" min_ver="71"/>
//     <value name="Render target 7" value="7" min_ver="71"/>
//     <value name="None" value="8"/>
//     <value name="Z" value="9"/>
//     <value name="Stencil" value="10"/>
//     <value name="Z+Stencil" value="11"/>
//   </field>
// </packet>
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
//   <field name="Index of First Vertex" size="32" start="40" type="uint"/>
//   <field name="Length" size="32" start="8" type="uint"/>
//   <field name="mode" size="8" start="0" type="Primitive"/>
// </packet>
struct [[gnu::packed]] VertexArrayPrims {
    u8 opcode = 36;
    u32 mode : 8;
    u32 length : 32;
    u32 index_of_first_vertex : 32;
};
static_assert(AssertSize<VertexArrayPrims, 1 + 9>());

// <packet code="54" name="Set InstanceID" cl="B">
//   <field name="Instance ID" size="32" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] SetInstanceID {
    u8 opcode = 54;
    u32 instance_id : 32;
};
static_assert(AssertSize<SetInstanceID, 1 + 4>());

// <packet code="56" name="Prim List Format">
//   <field name="tri strip or fan" size="1" start="7" type="bool"/>
//   <field name="primitive type" size="6" start="0" type="uint">
//     <value name="List Points" value="0"/>
//     <value name="List Lines" value="1"/>
//     <value name="List Triangles" value="2"/>
//   </field>
// </packet>
struct [[gnu::packed]] PrimListFormat {
    u8 opcode = 56;
    u8 primitive_type : 6;
    u8 _reserved0 : 1;
    u8 tri_strip_or_fan : 1;
};
static_assert(AssertSize<PrimListFormat, 1 + 1>());

// <packet code="64" shortname="gl_shader" name="GL Shader State">
//   <field name="address" size="27" start="5" type="address"/>
//   <field name="number of attribute arrays" size="5" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] GLShaderState {
    u8 opcode = 64;
    u32 number_of_attribute_arrays : 5;
    u32 address : 27;
};
static_assert(AssertSize<GLShaderState, 1 + 4>());

// <packet code="71" name="VCM Cache Size">
//   <field name="Number of 16-vertex batches for rendering" size="4" start="4" type="uint"/>
//   <field name="Number of 16-vertex batches for binning" size="4" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] VCMCacheSize {
    u8 opcode = 71;
    u8 number_of_16_vertex_batches_for_binning : 4;
    u8 number_of_16_vertex_batches_for_rendering : 4;
};
static_assert(AssertSize<VCMCacheSize, 1 + 1>());

// <packet code="74" name="Transform Feedback Specs">
//   <field name="Enable" size="1" start="7" type="bool"/>
//   <field name="Number of 16-bit Output Data Specs following" size="5" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] TransformFeedbackSpecs {
    u8 opcode = 74;
    u8 number_of_16bit_output_data_specs_following : 5;
    u8 _reserved0 : 1;
    u8 enable : 1;
};
static_assert(AssertSize<TransformFeedbackSpecs, 1 + 1>());

// <packet code="86" shortname="blend_ccolor" name="Blend Constant Color">
//   <field name="Alpha (F16)" size="16" start="48" type="uint"/>
//   <field name="Blue (F16)" size="16" start="32" type="uint"/>
//   <field name="Green (F16)" size="16" start="16" type="uint"/>
//   <field name="Red (F16)" size="16" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] BlendConstantColor {
    u8 opcode = 86;
    u64 red : 16;   // float?
    u64 green : 16; // float?
    u64 blue : 16;  // float?
    u64 alpha : 16; // float?
};
static_assert(AssertSize<BlendConstantColor, 1 + 8>());

// <packet code="87" shortname="color_wmasks" name="Color Write Masks">
//   <field name="Mask" size="32" start="0" type="uint"/>
// </packet>
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
//   <field name="Coverage" size="16" start="16" type="f187"/>
//   <field name="Mask" size="4" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] SampleState {
    u8 opcode = 91;
    u32 mask : 4;
    u32 _reserved0 : 12;
    u32 coverage : 16; // f187
};
static_assert(AssertSize<SampleState, 1 + 4>());

// <packet code="92" shortname="occlusion_query_counter_enable" name="Occlusion Query Counter">
//   <field name="address" size="32" start="0" type="address"/>
// </packet>
struct [[gnu::packed]] OcclusionQueryCounter {
    u8 opcode = 92;
    u32 address : 32;
};
static_assert(AssertSize<OcclusionQueryCounter, 1 + 4>());

// <packet code="96" name="Cfg Bits" min_ver="71">
//   <field name="Z Clipping mode" size="2" start="22" type="Z Clip Mode"/>
//   <field name="Direct3D Provoking Vertex" size="1" start="21" type="bool"/>
//   <field name="Direct3D 'Point-fill' mode" size="1" start="20" type="bool"/>
//   <field name="Blend enable" size="1" start="19" type="bool"/>
//   <field name="Stencil enable" size="1" start="18" type="bool"/>
//   <field name="Z updates enable" size="1" start="15" type="bool"/>
//   <field name="Depth-Test Function" size="3" start="12" type="Compare Function"/>
//   <field name="Direct3D Wireframe triangles mode" size="1" start="11" type="bool"/>
//   <field name="Z Clamp Mode" size="1" start="10" type="bool"/>
//   <field name="Rasterizer Oversample Mode" size="2" start="6" type="uint"/>
//   <field name="Depth Bounds Test Enable" size="1" start="5" type="bool"/>
//   <field name="Line Rasterization" size="1" start="4" type="uint"/>
//   <field name="Enable Depth Offset" size="1" start="3" type="bool"/>
//   <field name="Clockwise Primitives" size="1" start="2" type="bool"/>
//   <field name="Enable Reverse Facing Primitive" size="1" start="1" type="bool"/>
//   <field name="Enable Forward Facing Primitive" size="1" start="0" type="bool"/>
// </packet>
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
    u64 depth_test_function : 3;
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
//   <field name="Point Size" size="32" start="0" type="float"/>
// </packet>
struct [[gnu::packed]] PointSize {
    u8 opcode = 104;
    u32 point_size : 32; // float
};
static_assert(AssertSize<PointSize, 1 + 4>());

// <packet code="105" name="Line width">
//   <field name="Line width" size="32" start="0" type="float"/>
// </packet>
struct [[gnu::packed]] LineWidth {
    u8 opcode = 105;
    u32 line_width : 32; // float
};
static_assert(AssertSize<LineWidth, 1 + 4>());

// <packet shortname="clip" name="clip_window" code="107">
//   <field name="Clip Window Height in pixels" size="16" start="48" type="uint"/>
//   <field name="Clip Window Width in pixels" size="16" start="32" type="uint"/>
//   <field name="Clip Window Bottom Pixel Coordinate" size="16" start="16" type="uint"/>
//   <field name="Clip Window Left Pixel Coordinate" size="16" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] ClipWindow {
    u8 opcode = 107;
    u64 clip_window_left_pixel_coordinate : 16;
    u64 clip_window_bottom_pixel_coordinate : 16;
    u64 clip_window_width_in_pixels : 16;
    u64 clip_window_height_in_pixels : 16;
};
static_assert(AssertSize<ClipWindow, 1 + 8>());

// <packet name="Viewport Offset" code="108">
//   <field name="Coarse Y" size="10" start="54" type="int"/>
//   <field name="Fine Y" size="22" start="32" type="u14.8"/>
//   <field name="Coarse X" size="10" start="22" type="int"/>
//   <field name="Fine X" size="22" start="0" type="u14.8"/>
// </packet>
struct [[gnu::packed]] ViewportOffset {
    u8 opcode = 108;
    u64 fine_x : 22; // unsigned 14.8 fixed point
    i64 coarse_x : 10;
    u64 fine_y : 22; // unsigned 14.8 fixed point
    i64 coarse_y : 10;
};
static_assert(AssertSize<ViewportOffset, 1 + 8>());

// <packet shortname="clipz" name="Clipper Z min/max clipping planes" code="109">
//   <field name="Maximum Zw" size="32" start="32" type="float"/>
//   <field name="Minimum Zw" size="32" start="0" type="float"/>
// </packet>
struct [[gnu::packed]] ClipperZMinMaxClippingPlanes {
    u8 opcode = 109;
    u32 minimum_zw : 32; // float
    u32 maximum_zw : 32; // float
};
static_assert(AssertSize<ClipperZMinMaxClippingPlanes, 1 + 8>());

// <packet shortname="clipper_xy" name="Clipper XY Scaling" code="110" cl="B" min_ver="71">
//   <field name="Viewport Half-Height in 1/64th of pixel" size="32" start="32" type="float"/>
//   <field name="Viewport Half-Width in 1/64th of pixel" size="32" start="0" type="float"/>
// </packet>
struct [[gnu::packed]] ClipperXYScaling {
    u8 opcode = 110;
    u32 viewport_half_width_in_1_64th_of_pixel : 32;  // float
    u32 viewport_half_height_in_1_64th_of_pixel : 32; // float
};
static_assert(AssertSize<ClipperXYScaling, 1 + 8>());

// <packet shortname="clipper_z" name="Clipper Z Scale and Offset" code="111" cl="B">
//   <field name="Viewport Z Offset (Zc to Zs)" size="32" start="32" type="float"/>
//   <field name="Viewport Z Scale (Zc to Zs)" size="32" start="0" type="float"/>
// </packet>
struct [[gnu::packed]] ClipperZScaling {
    u8 opcode = 111;
    u32 viewport_z_scale : 32;  // float
    u32 viewport_z_offset : 32; // float
};
static_assert(AssertSize<ClipperZScaling, 1 + 8>());

// <packet name="Number of Layers" code="119">
//   <field name="Number of Layers" size="8" start="0" type="uint" minus_one="true"/>
// </packet>
struct [[gnu::packed]] NumberOfLayers {
    u8 opcode = 119;
    u8 number_of_layers_minus_one : 8;
};
static_assert(AssertSize<NumberOfLayers, 1 + 1>());

// <packet code="120" name="Tile Binning Mode Cfg" min_ver="71">
//   <field name="Height (in pixels)" size="16" start="48" type="uint" minus_one="true"/>
//   <field name="Width (in pixels)" size="16" start="32" type="uint" minus_one="true"/>
//
//   <field name="Log2 Tile Height" size="3" start="11" type="uint">
//     <value name="tile height 8 pixels" value="0"/>
//     <value name="tile height 16 pixels" value="1"/>
//     <value name="tile height 32 pixels" value="2"/>
//     <value name="tile height 64 pixels" value="3"/>
//   </field>
//   <field name="Log2 Tile Width"  size="3" start="8" type="uint">
//     <value name="tile width 8 pixels" value="0"/>
//     <value name="tile width 16 pixels" value="1"/>
//     <value name="tile width 32 pixels" value="2"/>
//     <value name="tile width 64 pixels" value="3"/>
//   </field>
//
//   <field name="tile allocation block size" size="2" start="4" type="uint">
//     <value name="tile allocation block size 64b" value="0"/>
//     <value name="tile allocation block size 128b" value="1"/>
//     <value name="tile allocation block size 256b" value="2"/>
//   </field>
//   <field name="tile allocation initial block size" size="2" start="2" type="uint">
//     <value name="tile allocation initial block size 64b" value="0"/>
//     <value name="tile allocation initial block size 128b" value="1"/>
//     <value name="tile allocation initial block size 256b" value="2"/>
//   </field>
// </packet>
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
//   <field name="Pad" size="6" start="58" type="uint"/>
//
//   <field name="Log2 Tile Height" size="3" start="55" type="uint">
//     <value name="tile height 8 pixels" value="0"/>
//     <value name="tile height 16 pixels" value="1"/>
//     <value name="tile height 32 pixels" value="2"/>
//     <value name="tile height 64 pixels" value="3"/>
//   </field>
//   <field name="Log2 Tile Width"  size="3" start="52" type="uint">
//     <value name="tile width 8 pixels" value="0"/>
//     <value name="tile width 16 pixels" value="1"/>
//     <value name="tile width 32 pixels" value="2"/>
//     <value name="tile width 64 pixels" value="3"/>
//   </field>
//
//   <field name="Early Depth/Stencil Clear" size="1" start="51" type="bool"/>
//   <field name="Internal Depth Type" size="4" start="47" type="Internal Depth Type"/>
//
//   <field name="Early-Z disable" size="1" start="46" type="bool"/>
//
//   <field name="Early-Z Test and Update Direction" size="1" start="45" type="uint">
//     <value name="Early-Z direction LT/LE" value="0"/>
//     <value name="Early-Z direction GT/GE" value="1"/>
//   </field>
//
//   <field name="Depth-buffer disable" size="1" start="44" type="bool"/>
//   <field name="Double-buffer in non-ms mode" size="1" start="43" type="bool"/>
//   <field name="Multisample Mode (4x)" size="1" start="42" type="bool"/>
//
//   <field name="Image Height (pixels)" size="16" start="24" type="uint"/>
//   <field name="Image Width (pixels)" size="16" start="8" type="uint"/>
//   <field name="Number of Render Targets" size="4" start="4" type="uint" minus_one="true"/>
//
//   <field name="sub-id" size="3" start="0" type="uint" default="0"/>
// </packet>
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
//   <field name="unused" size="16" start="48" type="uint"/>
//
//   <field name="Z Clear Value" size="32" start="16" type="float"/>
//
//   <field name="Stencil Clear Value" size="8" start="8" type="uint"/>
//   <field name="sub-id" size="4" start="0" type="uint" default="1"/>
// </packet>
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
//
//   <field name="Clear Color low bits" size="32" start="32" type="uint"/>
//   <field name="Internal Type and Clamping" size="5" start="27" type="Render Target Type Clamp"/>
//   <field name="Internal BPP" size="2" start="25" type="Internal BPP"/>
//
//   <field name="Stride" size="7" start="18" type="uint" minus_one="true"/>
//   <!-- In multiples of 512 bits -->
//   <field name="Base Address" size="11" start="7" type="uint"/>
//   <field name="Render Target number" size="3" start="3" type="uint"/>
//   <field name="sub-id" size="3" start="0" type="uint" default="2"/>
// </packet>
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
//   <field name="Number of Bin Tile Lists" size="3" start="61" type="uint" minus_one="true"/>
//   <field name="Supertile Raster Order" size="1" start="60" type="bool"/>
//   <field name="Multicore Enable" size="1" start="56" type="bool"/>
//
//   <field name="Total Frame Height in Tiles" size="12" start="44" type="uint"/>
//   <field name="Total Frame Width in Tiles" size="12" start="32" type="uint"/>
//
//   <field name="Total Frame Height in Supertiles" size="8" start="24" type="uint"/>
//   <field name="Total Frame Width in Supertiles" size="8" start="16" type="uint"/>
//
//   <field name="Supertile Height in Tiles" size="8" start="8" type="uint" minus_one="true"/>
//   <field name="Supertile Width in Tiles" size="8" start="0" type="uint" minus_one="true"/>
// </packet>
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
//   <field name="address" size="26" start="6" type="address"/>
//   <field name="Tile List Set Number" size="4" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] MulticoreRenderingTileListSetBase {
    u8 opcode = 123;
    u32 tile_list_set_number : 4;
    u32 _reserved0 : 2;
    u32 address : 26;
};
static_assert(AssertSize<MulticoreRenderingTileListSetBase, 1 + 4>());

// <packet code="124" shortname="tile_coords" name="Tile Coordinates">
//   <field name="tile row number" size="12" start="12" type="uint"/>
//   <field name="tile column number" size="12" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] TileCoordinates {
    u8 opcode = 124;
    u32 tile_column_number : 12;
    u32 tile_row_number : 12;
};
static_assert(AssertSize<TileCoordinates, 1 + 3>());

// <!-- add fields -->
// <packet code="125" shortname="implicit_tile_coords" name="Tile Coordinates Implicit"/>
struct [[gnu::packed]] ImplicitTileCoordinates {
    u8 opcode = 125;
};
static_assert(AssertSize<ImplicitTileCoordinates, 1>());

// <packet code="126" name="Tile List Initial Block Size">
//   <field name="Use auto-chained tile lists" size="1" start="2" type="bool"/>
//
//   <field name="Size of first block in chained tile lists" size="2" start="0" type="uint">
//     <value name="tile allocation block size 64b" value="0"/>
//     <value name="tile allocation block size 128b" value="1"/>
//     <value name="tile allocation block size 256b" value="2"/>
//   </field>
// </packet>
struct [[gnu::packed]] TileListInitialBlockSize {
    u8 opcode = 126;
    u8 size_of_first_block_in_chained_tile_lists : 2;
    u8 use_auto_chained_tile_lists : 1;
    u8 _reserved0 : 5;
};
static_assert(AssertSize<TileListInitialBlockSize, 1 + 1>());

// XXX: These aren't control lists

// <struct name="GL Shader State Record" min_ver="71">
//   <field name="Point size in shaded vertex data" size="1" start="0" type="bool"/>
//   <field name="Enable clipping" size="1" start="1" type="bool"/>
//
//   <field name="Vertex ID read by coordinate shader" size="1" start="2" type="bool"/>
//   <field name="Instance ID read by coordinate shader" size="1" start="3" type="bool"/>
//   <field name="Base Instance ID read by coordinate shader" size="1" start="4" type="bool"/>
//   <field name="Vertex ID read by vertex shader" size="1" start="5" type="bool"/>
//   <field name="Instance ID read by vertex shader" size="1" start="6" type="bool"/>
//   <field name="Base Instance ID read by vertex shader" size="1" start="7" type="bool"/>
//
//   <field name="Fragment shader does Z writes" size="1" start="8" type="bool"/>
//   <field name="Turn off early-z test" size="1" start="9" type="bool"/>
//
//   <field name="Fragment shader uses real pixel centre W in addition to centroid W2" size="1" start="12" type="bool"/>
//   <field name="Enable Sample Rate Shading" size="1" start="13" type="bool"/>
//   <field name="Any shader reads hardware-written Primitive ID" size="1" start="14" type="bool"/>
//   <field name="Insert Primitive ID as first varying to fragment shader" size="1" start="15" type="bool"/>
//   <field name="Turn off scoreboard" size="1" start="16" type="bool"/>
//   <field name="Do scoreboard wait on first thread switch" size="1" start="17" type="bool"/>
//   <field name="Disable implicit point/line varyings" size="1" start="18" type="bool"/>
//   <field name="No prim pack" size="1" start="19" type="bool"/>
//   <field name="Never defer FEP depth writes" size="1" start="20" type="bool"/>
//
//   <field name="Number of varyings in Fragment Shader" size="8" start="3b" type="uint"/>
//
//   <field name="Coordinate Shader output VPM segment size" size="4" start="4b" type="uint"/>
//   <field name="Min Coord Shader output segments required in play in addition to VCM cache size" size="4" start="36" type="uint"/>
//
//   <field name="Coordinate Shader input VPM segment size" size="4" start="5b" type="uint"/>
//   <field name="Min Coord Shader input segments required in play" size="4" start="44" type="uint" minus_one="true"/>
//
//   <field name="Vertex Shader output VPM segment size" size="4" start="6b" type="uint"/>
//   <field name="Min Vertex Shader output segments required in play in addition to VCM cache size" size="4" start="52" type="uint"/>
//
//   <field name="Vertex Shader input VPM segment size" size="4" start="7b" type="uint"/>
//   <field name="Min Vertex Shader input segments required in play" size="4" start="60" type="uint" minus_one="true"/>
//
//   <field name="Fragment Shader Code Address" size="29" start="67" type="address"/>
//   <field name="Fragment Shader 4-way threadable" size="1" start="64" type="bool"/>
//   <field name="Fragment Shader start in final thread section" size="1" start="65" type="bool"/>
//   <field name="Fragment Shader Propagate NaNs" size="1" start="66" type="bool"/>
//   <field name="Fragment Shader Uniforms Address" size="32" start="12b" type="address"/>
//
//   <field name="Vertex Shader Code Address" size="29" start="131" type="address"/>
//   <field name="Vertex Shader 4-way threadable" size="1" start="128" type="bool"/>
//   <field name="Vertex Shader start in final thread section" size="1" start="129" type="bool"/>
//   <field name="Vertex Shader Propagate NaNs" size="1" start="130" type="bool"/>
//   <field name="Vertex Shader Uniforms Address" size="32" start="20b" type="address"/>
//
//   <field name="Coordinate Shader Code Address" size="29" start="195" type="address"/>
//   <field name="Coordinate Shader 4-way threadable" size="1" start="192" type="bool"/>
//   <field name="Coordinate Shader start in final thread section" size="1" start="193" type="bool"/>
//   <field name="Coordinate Shader Propagate NaNs" size="1" start="194" type="bool"/>
//   <field name="Coordinate Shader Uniforms Address" size="32" start="28b" type="address"/>
// </struct>
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
//   <field name="Address" size="32" start="0" type="address"/>
//
//   <field name="Vec size" size="2" start="32" type="uint"/>
//   <field name="Type" size="3" start="34" type="uint">
//     <value name="Attribute half-float" value="1"/>
//     <value name="Attribute float" value="2"/>
//     <value name="Attribute fixed" value="3"/>
//     <value name="Attribute byte" value="4"/>
//     <value name="Attribute short" value="5"/>
//     <value name="Attribute int" value="6"/>
//     <value name="Attribute int2_10_10_10" value="7"/>
//   </field>
//   <field name="Signed int type" size="1" start="37" type="bool"/>
//   <field name="Normalized int type" size="1" start="38" type="bool"/>
//   <field name="Read as int/uint" size="1" start="39" type="bool"/>
//
//   <field name="Number of values read by Coordinate shader" size="4" start="40" type="uint"/>
//   <field name="Number of values read by Vertex shader" size="4" start="44" type="uint"/>
//
//   <field name="Instance Divisor" size="16" start="6b" type="uint"/>
//   <field name="Stride" size="32" start="8b" type="uint"/>
//   <field name="Maximum Index" size="32" start="12b" type="uint"/>
// </struct>
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
