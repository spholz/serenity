/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Array.h>
#include <AK/Types.h>

namespace V3DGPU {

// Because we don't have our own shader compiler yet, these shaders are precompiled by Mesa.

// Source vertex shader
// #version 300 es
//
// layout(location = 0) in vec3 i_pos;
// layout(location = 1) in vec3 i_color;
//
// uniform mat4 u_mvp;
//
// out vec3 f_color;
//
// void main() {
//     gl_Position = u_mvp * vec4(i_pos, 1.0);
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
static constexpr auto VERTEX_SHADER = to_array<u64>({
    0x38403186bb03f000, // nop                           ; nop                         ; ldunif (0x00000000 / 0.000000)
    0x38402184bc03f000, // ldvpmv_in rf4, rf0            ; nop                         ; ldunif (0x00000001 / 0.000000)
    0x38402185bc03f000, // ldvpmv_in rf5, rf0            ; nop                         ; ldunif (0x00000002 / 0.000000)
    0x3981e186bc03f000, // ldvpmv_in rf6, rf0            ; nop                         ; ldunifrf.rf7 (push[0])
    0x55821286bb1c4000, // nop                           ; fmul rf10, rf7, rf4         ; ldunifrf.rf8 (push[1])
    0x55825486bb204000, // nop                           ; fmul rf18, rf8, rf4         ; ldunifrf.rf9 (push[2])
    0x544015c6bb244000, // nop                           ; fmul rf23, rf9, rf4         ; ldunif (push[3])
    0x558312c6bb004000, // nop                           ; fmul rf11, rf0, rf4         ; ldunifrf.rf12 (push[4])
    0x558353c6bb305000, // nop                           ; fmul rf15, rf12, rf5        ; ldunifrf.rf13 (push[5])
    0x558384d40534528f, // fadd rf20, rf10, rf15         ; fmul rf19, rf13, rf5        ; ldunifrf.rf14 (push[6])
    0x384021960503f493, // fadd rf22, rf18, rf19         ; nop                         ; ldunif (push[7])
    0x54401406bb005000, // nop                           ; fmul rf16, rf0, rf5         ; ldunif (0x00000003 / 0.000000)
    0x55860151bc385000, // ldvpmv_in rf17, rf0           ; fmul rf5, rf14, rf5         ; ldunifrf.rf24 (push[8])
    0x558646d5056062d0, // fadd rf21, rf11, rf16         ; fmul rf27, rf24, rf6        ; ldunifrf.rf25 (push[9])
    0x558682010564651b, // fadd rf1, rf20, rf27          ; fmul rf8, rf25, rf6         ; ldunifrf.rf26 (push[10])
    0x5440050f05686216, // fadd rf15, rf8, rf22          ; fmul rf20, rf26, rf6        ; ldunif (push[11])
    0x55875706bb006000, // nop                           ; fmul rf28, rf0, rf6         ; ldunifrf.rf29 (0x00000004 / 0.000000)
    0x3840219ebc03f740, // ldvpmv_in rf30, rf29          ; nop                         ; ldunif (0x00000005 / 0.000000)
    0x3840219fbc03f000, // ldvpmv_in rf31, rf0           ; nop                         ; ldunif (0x00000006 / 0.000000)
    0x38402182bc03f000, // ldvpmv_in rf2, rf0            ; nop                         ; ldunif (0x00000007 / 0.000000)
    0x38402184bc03f000, // ldvpmv_in rf4, rf0            ; nop                         ; ldunif (0x00000008 / 0.000000)
    0x39826187bc03f000, // ldvpmv_in rf7, rf0            ; nop                         ; ldunifrf.rf9 (push[12])
    0x55828300be251744, // stvpmv rf29, rf4              ; fmul rf12, rf9, rf17        ; ldunifrf.rf10 (push[13])
    0x5582c4030529155c, // fadd rf3, rf21, rf28          ; fmul rf16, rf10, rf17       ; ldunifrf.rf11 (push[14])
    0x54400692052d104c, // fadd rf18, rf1, rf12          ; fmul rf26, rf11, rf17       ; ldunif (push[15])
    0x5583835505011157, // fadd rf21, rf5, rf23          ; fmul rf13, rf0, rf17        ; ldunifrf.rf14 (vp_x_scale)
    0x544006130548e0cd, // fadd rf19, rf3, rf13          ; fmul rf24, rf18, rf14       ; ldunif (0x00000005 / 0.000000)
    0x3985a180be03f007, // stvpmv rf0, rf7               ; nop                         ; ldunifrf.rf22 (vp_y_scale)
    0x38402199bc03f4e0, // recip rf25, rf19              ; nop                         ; ldunif (0x00000006 / 0.000000)
    0x3987a180be03f01e, // stvpmv rf0, rf30              ; nop                         ; ldunifrf.rf30 (vp_z_scale)
    0x54400757056193d0, // fadd rf23, rf15, rf16         ; fmul rf29, rf24, rf25       ; ldunif (0x00000007 / 0.000000)
    0x54400700be5d601f, // stvpmv rf0, rf31              ; fmul rf28, rf23, rf22       ; ldunif (0x00000008 / 0.000000)
    0x558180c0be719002, // stvpmv rf0, rf2               ; fmul rf3, rf28, rf25        ; ldunifrf.rf6 (vp_z_offset)
    0x3840219b0503f515, // fadd rf27, rf20, rf21         ; nop                         ; ldunif (0x00000000 / 0.000000)
    0x38002185f503f747, // ftoin rf5, rf29               ; nop
    0x380021810503f69b, // fadd rf1, rf26, rf27          ; nop
    0x54400100be05e005, // stvpmv rf0, rf5               ; fmul rf4, rf1, rf30         ; ldunif (0x00000001 / 0.000000)
    0x540001c8f51190c7, // ftoin rf8, rf3                ; fmul rf7, rf4, rf25
    0x38402180be03f008, // stvpmv rf0, rf8               ; nop                         ; ldunif (0x00000002 / 0.000000)
    0x380021890503f187, // fadd rf9, rf6, rf7            ; nop
    0x38602180be03f009, // stvpmv rf0, rf9               ; nop                         ; thrsw; ldunif (0x00000003 / 0.000000)
    0x38002180be03f019, // stvpmv rf0, rf25              ; nop
    0x38003186bb03f000, // nop                           ; nop
});

static constexpr auto COORDINATE_SHADER = to_array<u64>({
    0x38403186bb03f000, // nop                           ; nop                         ; ldunif (0x00000000 / 0.000000)
    0x38402184bc03f000, // ldvpmv_in rf4, rf0            ; nop                         ; ldunif (0x00000001 / 0.000000)
    0x38402185bc03f000, // ldvpmv_in rf5, rf0            ; nop                         ; ldunif (0x00000002 / 0.000000)
    0x3981e186bc03f000, // ldvpmv_in rf6, rf0            ; nop                         ; ldunifrf.rf7 (push[0])
    0x55821286bb1c4000, // nop                           ; fmul rf10, rf7, rf4         ; ldunifrf.rf8 (push[1])
    0x55825486bb204000, // nop                           ; fmul rf18, rf8, rf4         ; ldunifrf.rf9 (push[2])
    0x54401586bb244000, // nop                           ; fmul rf22, rf9, rf4         ; ldunif (push[3])
    0x558312c6bb004000, // nop                           ; fmul rf11, rf0, rf4         ; ldunifrf.rf12 (push[4])
    0x558353c6bb305000, // nop                           ; fmul rf15, rf12, rf5        ; ldunifrf.rf13 (push[5])
    0x558384d40534528f, // fadd rf20, rf10, rf15         ; fmul rf19, rf13, rf5        ; ldunifrf.rf14 (push[6])
    0x5440071e05385493, // fadd rf30, rf18, rf19         ; fmul rf28, rf14, rf5        ; ldunif (push[7])
    0x54401406bb005000, // nop                           ; fmul rf16, rf0, rf5         ; ldunif (0x00000003 / 0.000000)
    0x3985e191bc03f000, // ldvpmv_in rf17, rf0           ; nop                         ; ldunifrf.rf23 (push[8])
    0x55860695055c62d0, // fadd rf21, rf11, rf16         ; fmul rf26, rf23, rf6        ; ldunifrf.rf24 (push[9])
    0x558647450560651a, // fadd rf5, rf20, rf26          ; fmul rf29, rf24, rf6        ; ldunifrf.rf25 (push[10])
    0x5440024c0564675e, // fadd rf12, rf29, rf30         ; fmul rf9, rf25, rf6         ; ldunif (push[11])
    0x5587c6cb0500659c, // fadd rf11, rf22, rf28         ; fmul rf27, rf0, rf6         ; ldunifrf.rf31 (push[12])
    0x558040c7057d155b, // fadd rf7, rf21, rf27          ; fmul rf3, rf31, rf17        ; ldunifrf.rf1 (push[13])
    0x5580828d050510c5, // fadd rf13, rf3, rf5           ; fmul rf10, rf1, rf17        ; ldunifrf.rf2 (push[14])
    0x544003d20509128c, // fadd rf18, rf10, rf12         ; fmul rf15, rf2, rf17        ; ldunif (push[15])
    0x558201110501124b, // fadd rf17, rf9, rf11          ; fmul rf4, rf0, rf17         ; ldunifrf.rf8 (vp_x_scale)
    0x558404ce05348107, // fadd rf14, rf4, rf7           ; fmul rf19, rf13, rf8        ; ldunifrf.rf16 (vp_y_scale)
    0x54400595054903d1, // fadd rf21, rf15, rf17         ; fmul rf22, rf18, rf16       ; ldunif (0x00000000 / 0.000000)
    0x38402180be03f00d, // stvpmv rf0, rf13              ; nop                         ; ldunif (0x00000001 / 0.000000)
    0x38402180be03f012, // stvpmv rf0, rf18              ; nop                         ; ldunif (0x00000002 / 0.000000)
    0x38002194bc03f3a0, // recip rf20, rf14              ; nop
    0x38402180be03f015, // stvpmv rf0, rf21              ; nop                         ; ldunif (0x00000003 / 0.000000)
    0x544005c0be4d400e, // stvpmv rf0, rf14              ; fmul rf23, rf19, rf20       ; ldunif (0x00000004 / 0.000000)
    0x54000619f55945c7, // ftoin rf25, rf23              ; fmul rf24, rf22, rf20
    0x38602180be03f019, // stvpmv rf0, rf25              ; nop                         ; thrsw; ldunif (0x00000005 / 0.000000)
    0x3800219af503f607, // ftoin rf26, rf24              ; nop
    0x38002180be03f01a, // stvpmv rf0, rf26              ; nop
});

static constexpr auto FRAGMENT_SHADER = to_array<u64>({
    0x3a813186bb03f000, // nop                           ; nop                         ; ldvary.rf4; wrtmuc (tex[0].p0 | 0x3)
    0x5681d146bb103000, // nop                           ; fmul rf5, rf4, rf3          ; ldvary.rf7; wrtmuc (tex[0].p1 | 0x4)
    0x55028206051c3005, // fadd rf6, rf0, rf5            ; fmul rf8, rf7, rf3          ; ldvary.rf10
    0x550342c905283008, // fadd rf9, rf0, rf8            ; fmul rf11, rf10, rf3        ; ldvary.rf13
    0x3820288c0524300b, // fadd rf12, rf0, rf11          ; mov tmut, rf9               ; thrsw
    0x552413a1f9343183, // mov tmus, rf6                 ; fmul rf14, rf13, rf3        ; thrsw; ldvary.rf16
    0x5400044f0540300e, // fadd rf15, rf0, rf14          ; fmul rf17, rf16, rf3
    0x3884e1920503f011, // fadd rf18, rf0, rf17          ; nop                         ; ldtmu.rf19
    0x64851586bb4cc000, // nop                           ; fmul rf22, rf19.l, rf12     ; ldtmu.rf20
    0x740015c6bb4cf000, // nop                           ; fmul rf23, rf19.h, rf15
    0x6400154735512597, // vfpack tlb, rf22, rf23        ; fmul rf21, rf20.l, rf18
    0x382031873703f554, // vfpack tlb, rf21, rf20.h      ; nop                         ; thrsw
    0x38003186bb03f000, // nop                           ; nop
    0x38003186bb03f000, // nop                           ; nop
});

static constexpr auto FRAGMENT_SHADER_ALPHA_TEST_GREATER = to_array<u64>({
    0x3a813186bb03f000, // nop                           ; nop                         ; ldvary.rf4; wrtmuc (tex[0].p0 | 0x3)
    0x5681d146bb103000, // nop                           ; fmul rf5, rf4, rf3          ; ldvary.rf7; wrtmuc (tex[0].p1 | 0x0)
    0x55028206051c3005, // fadd rf6, rf0, rf5            ; fmul rf8, rf7, rf3          ; ldvary.rf10
    0x550342c905283008, // fadd rf9, rf0, rf8            ; fmul rf11, rf10, rf3        ; ldvary.rf13
    0x5524038c0534300b, // fadd rf12, rf0, rf11          ; fmul rf14, rf13, rf3        ; thrsw; ldvary.rf16
    0x3820288f0524300e, // fadd rf15, rf0, rf14          ; mov tmut, rf9               ; thrsw
    0x54001461f9403183, // mov tmus, rf6                 ; fmul rf17, rf16, rf3
    0x38c4e1920503f011, // fadd rf18, rf0, rf17          ; nop                         ; ldtmu.rf19; ldunif (push[0])
    0x64851586bb4cc000, // nop                           ; fmul rf22, rf19.l, rf12     ; ldtmu.rf20
    0x7400d5c6cd4cf500, // fcmp.pushc -, rf20.h, rf0     ; fmul rf23, rf19.h, rf15
    0x65c81546ba512006, // setmsf.ifa -, 0               ; fmul rf21, rf20.l, rf18
    0x38203206bb143000, // nop                           ; mov tlbu, rf5               ; thrsw (0xffffff84 / -nan)
    0x380031873503f597, // vfpack tlb, rf22, rf23        ; nop
    0x380031873703f554, // vfpack tlb, rf21, rf20.h      ; nop
});
// clang-format on

}
