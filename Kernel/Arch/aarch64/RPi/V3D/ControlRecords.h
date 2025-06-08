/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>

namespace Kernel::RPi::V3D::ControlRecord {

// <packet code="4" name="Flush"/>
struct [[gnu::packed]] Flush {
    u8 opcode = 4;
};
static_assert(sizeof(Flush) == 1);

// <packet code="6" name="Start Tile Binning"/>
struct [[gnu::packed]] StartTileBinning {
    u8 opcode = 6;
};
static_assert(sizeof(StartTileBinning) == 1);

// <packet code="13" shortname="end_render" name="End of rendering"/>
struct [[gnu::packed]] EndOfRendering {
    u8 opcode = 13;
};
static_assert(sizeof(EndOfRendering) == 1);

// <packet code="18" shortname="return" name="Return from sub-list"/>
struct [[gnu::packed]] ReturnFromSubList {
    u8 opcode = 18;
};
static_assert(sizeof(ReturnFromSubList) == 1);

// <packet code="19" shortname="clear_vcd_cache" name="Flush VCD cache"/>
struct [[gnu::packed]] FlushVCDCache {
    u8 opcode = 19;
};
static_assert(sizeof(FlushVCDCache) == 1);

// <packet code="20" shortname="generic_tile_list" name="Start Address of Generic Tile List">
//   <field name="start" size="32" start="0" type="address"/>
//   <field name="end" size="32" start="32" type="address"/>
// </packet>
struct [[gnu::packed]] StartAddressOfGenericTileList {
    u8 opcode = 20;
    u32 start : 32;
    u32 end : 32;
};
static_assert(sizeof(StartAddressOfGenericTileList) == 1 + 8);

// <packet code="21" shortname="branch_implicit_tile" name="Branch to Implicit Tile List">
//   <field name="tile list set number" size="8" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] BranchToImplictiTileList {
    u8 opcode = 21;
    u8 tile_list_set_number : 8;
};
static_assert(sizeof(BranchToImplictiTileList) == 1 + 1);

// <packet code="23" shortname="supertile_coords" name="Supertile Coordinates">
//   <field name="row number in supertiles" size="8" start="8" type="uint"/>
//   <field name="column number in supertiles" size="8" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] SupertileCoordinates {
    u8 opcode = 23;
    u16 column_number_in_supertiles : 8;
    u16 row_number_in_supertiles : 8;
};
static_assert(sizeof(SupertileCoordinates) == 1 + 2);

// <packet code="25" shortname="clear_rt" name="Clear Render Targets" cl="R" min_ver="71"/>
struct [[gnu::packed]] ClearRenderTargets {
    u8 opcode = 25;
};
static_assert(sizeof(ClearRenderTargets) == 1);

// <packet code="26" shortname="end_loads" name="End of Loads" cl="R"/>
struct [[gnu::packed]] EndOfLoads {
    u8 opcode = 26;
};
static_assert(sizeof(EndOfLoads) == 1);

// <packet code="27" shortname="end_tile" name="End of Tile Marker" cl="R"/>
struct [[gnu::packed]] EndOfTileMarker {
    u8 opcode = 27;
};
static_assert(sizeof(EndOfTileMarker) == 1);

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
    u64 : 7;
    u64 height_in_ub_or_stride : 20;
    u64 height : 16;
    u64 address : 32;
};
static_assert(sizeof(StoreTileBufferGeneral) == 1 + 12);

// <packet code="54" name="Set InstanceID" cl="B">
//   <field name="Instance ID" size="32" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] SetInstanceID {
    u8 opcode = 54;
    u32 instance_id : 32;
};
static_assert(sizeof(SetInstanceID) == 1 + 4);

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
    u8 : 1;
    u8 tri_strip_or_fan : 1;
};
static_assert(sizeof(PrimListFormat) == 1 + 1);

// <packet code="92" shortname="occlusion_query_counter_enable" name="Occlusion Query Counter">
//   <field name="address" size="32" start="0" type="address"/>
// </packet>
struct [[gnu::packed]] OcclusionQueryCounter {
    u8 opcode = 92;
    u32 address : 32;
};
static_assert(sizeof(OcclusionQueryCounter) == 1 + 4);

// <packet name="Number of Layers" code="119">
//   <field name="Number of Layers" size="8" start="0" type="uint" minus_one="true"/>
// </packet>
struct [[gnu::packed]] NumberOfLayers {
    u8 opcode = 119;
    u8 number_of_layers_minus_one : 8;
};
static_assert(sizeof(NumberOfLayers) == 1 + 1);

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
    u64 : 2;
    u64 tile_allocation_initial_block_size : 2;
    u64 tile_allocation_block_size : 2;
    u64 : 2;
    u64 log2_tile_width : 3;
    u64 log2_tile_height : 3;
    u64 : 18;
    u64 width_in_pixels_minus_one : 16;
    u64 height_in_pixels_minus_one : 16;
};
static_assert(sizeof(TileBinningModeCfg) == 9);

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
    u64 : 1;
    u64 number_of_render_targets_minus_one : 4;
    u64 image_width_pixels : 16;
    u64 image_height_pixels : 16;
    u64 : 2;
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
static_assert(sizeof(TileRenderingModeCfgCommon) == 1 + 8);

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
    u8 : 4;
    u64 stencil_clear_value : 8;
    u64 z_clear_value : 32; // float
    u64 unused : 16;
};
static_assert(sizeof(TileRenderingModeCfgZSClearValues) == 1 + 8);

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
    u64 : 1;
    u64 base_address : 11;
    u64 stride_minus_one : 7;
    u64 internal_bpp : 2;
    u64 internal_type_and_clamping : 5;
    u64 clear_color_low_bits : 32;
};
static_assert(sizeof(TileRenderingModeCfgRenderTargetPart1) == 1 + 8);

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
    u64 : 3;
    u64 supertile_raster_order : 1;
    u64 number_of_bin_tile_lists_minus_one : 3;
};
static_assert(sizeof(MulticoreRenderingSupertileCfg) == 1 + 8);

// <packet code="123" shortname="multicore_rendering_tile_list_base" name="Multicore Rendering Tile List Set Base" cl="R">
//   <field name="address" size="26" start="6" type="address"/>
//   <field name="Tile List Set Number" size="4" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] MulticoreRenderingTileListSetBase {
    u8 opcode = 123;
    u32 tile_list_set_number : 4;
    u32 : 2;
    u32 address : 26;
};
static_assert(sizeof(MulticoreRenderingTileListSetBase) == 1 + 4);

// <packet code="124" shortname="tile_coords" name="Tile Coordinates">
//   <field name="tile row number" size="12" start="12" type="uint"/>
//   <field name="tile column number" size="12" start="0" type="uint"/>
// </packet>
struct [[gnu::packed]] TileCoordinates {
    u8 opcode = 124;
    u32 tile_column_number : 12;
    u32 tile_row_number : 12;
};
static_assert(sizeof(TileCoordinates) == 1 + 3);

// <!-- add fields -->
// <packet code="125" shortname="implicit_tile_coords" name="Tile Coordinates Implicit"/>
struct [[gnu::packed]] ImplicitTileCoordinates {
    u8 opcode = 125;
};
static_assert(sizeof(ImplicitTileCoordinates) == 1);

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
    u8 : 5;
};
static_assert(sizeof(TileListInitialBlockSize) == 1 + 1);

}
