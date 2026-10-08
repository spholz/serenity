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
    0x3981f186bb03f000, // nop                           ; nop                         ; ldunifrf.rf7 (push[0])
    0x39823186bb03f000, // nop                           ; nop                         ; ldunifrf.rf8 (push[1])
    0x39827186bb03f000, // nop                           ; nop                         ; ldunifrf.rf9 (push[2])
    0x38403186bb03f000, // nop                           ; nop                         ; ldunif (push[3])
    0x39833186bb03f000, // nop                           ; nop                         ; ldunifrf.rf12 (push[4])
    0x39c02184bc03f000, // ldvpmv_in rf4, 0              ; nop
    0x558352c6bb004000, // nop                           ; fmul rf11, rf0, rf4         ; ldunifrf.rf13 (push[5])
    0x55839286bb1c4000, // nop                           ; fmul rf10, rf7, rf4         ; ldunifrf.rf14 (push[6])
    0x54401446bb204000, // nop                           ; fmul rf17, rf8, rf4         ; ldunif (push[7])
    0x5584d786bb244000, // nop                           ; fmul rf30, rf9, rf4         ; ldunifrf.rf19 (push[8])
    0x39c02185bc03f040, // ldvpmv_in rf5, 1              ; nop
    0x55851406bb005000, // nop                           ; fmul rf16, rf0, rf5         ; ldunifrf.rf20 (push[9])
    0x558543d9053052d0, // fadd rf25, rf11, rf16         ; fmul rf15, rf12, rf5        ; ldunifrf.rf21 (push[10])
    0x544004970534528f, // fadd rf23, rf10, rf15         ; fmul rf18, rf13, rf5        ; ldunif (push[11])
    0x5587c05b05385452, // fadd rf27, rf17, rf18         ; fmul rf1, rf14, rf5         ; ldunifrf.rf31 (vp_x_scale)
    0x3980e18d0503f05e, // fadd rf13, rf1, rf30          ; nop                         ; ldunifrf.rf3 (push[12])
    0x39c02186bc03f080, // ldvpmv_in rf6, 2              ; nop
    0x55811606bb006000, // nop                           ; fmul rf24, rf0, rf6         ; ldunifrf.rf4 (push[13])
    0x5581459d054c6619, // fadd rf29, rf24, rf25         ; fmul rf22, rf19, rf6        ; ldunifrf.rf5 (push[14])
    0x55c00689bc5060c0, // ldvpmv_in rf9, 3              ; fmul rf26, rf20, rf6
    0x5440031c05546597, // fadd rf28, rf22, rf23         ; fmul rf12, rf21, rf6        ; ldunif (push[15])
    0x380021820503f69b, // fadd rf2, rf26, rf27          ; nop
    0x39c0218abc03f100, // ldvpmv_in rf10, 4             ; nop
    0x384021880503f01d, // fadd rf8, rf0, rf29           ; nop                         ; ldunif (vp_y_scale)
    0x380021870503f0dc, // fadd rf7, rf3, rf28           ; nop
    0x540003ce051df084, // fadd rf14, rf2, rf4           ; fmul rf15, rf7, rf31
    0x55c0048bbc380140, // ldvpmv_in rf11, 5             ; fmul rf18, rf14, rf0
    0x38402190bc03f220, // recip rf16, rf8               ; nop                         ; ldunif (vp_z_scale)
    0x380021910503f30d, // fadd rf17, rf12, rf13         ; nop
    0x55c004c0be3d0109, // stvpmv 4, rf9                 ; fmul rf19, rf15, rf16
    0x5400055405490151, // fadd rf20, rf5, rf17          ; fmul rf21, rf18, rf16
    0x55c00580be50014a, // stvpmv 5, rf10                ; fmul rf22, rf20, rf0
    0x54400617f55904c7, // ftoin rf23, rf19              ; fmul rf24, rf22, rf16       ; ldunif (vp_z_offset)
    0x39c02180be03f18b, // stvpmv 6, rf11                ; nop
    0x38002199f503f547, // ftoin rf25, rf21              ; nop
    0x39c02180be03f017, // stvpmv 0, rf23                ; nop
    0x3800219a0503f018, // fadd rf26, rf0, rf24          ; nop
    0x39c02180be03f059, // stvpmv 1, rf25                ; nop
    0x39c02180be03f09a, // stvpmv 2, rf26                ; nop
    0x39c02180be03f0d0, // stvpmv 3, rf16                ; nop
    0x38203186bb03f000, // nop                           ; nop                         ; thrsw
    0x38003186bb03f000, // nop                           ; nop
    0x38003186bb03f000, // nop                           ; nop
});

static constexpr auto COORDINATE_SHADER = to_array<u64>({
    0x3981f186bb03f000, // nop                           ; nop                         ; ldunifrf.rf7 (push[0])
    0x39823186bb03f000, // nop                           ; nop                         ; ldunifrf.rf8 (push[1])
    0x39827186bb03f000, // nop                           ; nop                         ; ldunifrf.rf9 (push[2])
    0x38403186bb03f000, // nop                           ; nop                         ; ldunif (push[3])
    0x39833186bb03f000, // nop                           ; nop                         ; ldunifrf.rf12 (push[4])
    0x39c02184bc03f000, // ldvpmv_in rf4, 0              ; nop
    0x558352c6bb004000, // nop                           ; fmul rf11, rf0, rf4         ; ldunifrf.rf13 (push[5])
    0x55839286bb1c4000, // nop                           ; fmul rf10, rf7, rf4         ; ldunifrf.rf14 (push[6])
    0x54401446bb204000, // nop                           ; fmul rf17, rf8, rf4         ; ldunif (push[7])
    0x5584d686bb244000, // nop                           ; fmul rf26, rf9, rf4         ; ldunifrf.rf19 (push[8])
    0x39c02185bc03f040, // ldvpmv_in rf5, 1              ; nop
    0x55851406bb005000, // nop                           ; fmul rf16, rf0, rf5         ; ldunifrf.rf20 (push[9])
    0x558543d9053052d0, // fadd rf25, rf11, rf16         ; fmul rf15, rf12, rf5        ; ldunifrf.rf21 (push[10])
    0x544004970534528f, // fadd rf23, rf10, rf15         ; fmul rf18, rf13, rf5        ; ldunif (push[11])
    0x55c006c6bc385080, // ldvpmv_in rf6, 2              ; fmul rf27, rf14, rf5
    0x5580459d054c6452, // fadd rf29, rf17, rf18         ; fmul rf22, rf19, rf6        ; ldunifrf.rf1 (vp_x_scale)
    0x5581461e05006597, // fadd rf30, rf22, rf23         ; fmul rf24, rf0, rf6         ; ldunifrf.rf5 (push[12])
    0x5400071f05506619, // fadd rf31, rf24, rf25         ; fmul rf28, rf20, rf6
    0x558180830554669b, // fadd rf3, rf26, rf27          ; fmul rf2, rf21, rf6         ; ldunifrf.rf6 (push[13])
    0x3981e1880503f15e, // fadd rf8, rf5, rf30           ; nop                         ; ldunifrf.rf7 (push[14])
    0x544003040520171d, // fadd rf4, rf28, rf29          ; fmul rf12, rf8, rf1         ; ldunif (push[15])
    0x384021890503f01f, // fadd rf9, rf0, rf31           ; nop                         ; ldunif (vp_y_scale)
    0x3800218a0503f083, // fadd rf10, rf2, rf3           ; nop
    0x39c02180be03f008, // stvpmv 0, rf8                 ; nop
    0x3800218b0503f106, // fadd rf11, rf4, rf6           ; nop
    0x540003cdbc2c0260, // recip rf13, rf9               ; fmul rf15, rf11, rf0
    0x3800218e0503f1ca, // fadd rf14, rf7, rf10          ; nop
    0x55c00400be30d04b, // stvpmv 1, rf11                ; fmul rf16, rf12, rf13
    0x55c00440be3cd08e, // stvpmv 2, rf14                ; fmul rf17, rf15, rf13
    0x38002192f503f407, // ftoin rf18, rf16              ; nop
    0x39c02180be03f0c9, // stvpmv 3, rf9                 ; nop
    0x38002193f503f447, // ftoin rf19, rf17              ; nop
    0x39c02180be03f112, // stvpmv 4, rf18                ; nop
    0x39c02180be03f153, // stvpmv 5, rf19                ; nop
    0x38203186bb03f000, // nop                           ; nop                         ; thrsw
    0x38003186bb03f000, // nop                           ; nop
    0x38003186bb03f000, // nop                           ; nop
});

static constexpr auto FRAGMENT_SHADER = to_array<u64>({
    0x39013186bb03f000, // nop                           ; nop                         ; ldvary.rf4
    0x5521d146bb103000, // nop                           ; fmul rf5, rf4, rf3          ; thrsw; ldvary.rf7
    0x55228206051c3005, // fadd rf6, rf0, rf5            ; fmul rf8, rf7, rf3          ; thrsw; ldvary.rf10
    0x540002c905283008, // fadd rf9, rf0, rf8            ; fmul rf11, rf10, rf3
    0x3800218c0503f00b, // fadd rf12, rf0, rf11          ; nop
    0x382031873503f189, // vfpack tlb, rf6, rf9          ; nop                         ; thrsw
    0x39e031873503f328, // vfpack tlb, rf12, 0x3f800000  ; nop
    0x38003186bb03f000, // nop                           ; nop
});
// clang-format on

}
