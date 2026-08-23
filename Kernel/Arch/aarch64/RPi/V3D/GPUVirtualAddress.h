/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/DistinctNumeric.h>
#include <AK/Types.h>

namespace Kernel::RPi::V3D {

AK_TYPEDEF_DISTINCT_NUMERIC_GENERAL(u32, GPUVirtualAddress);

// class GPUVirtualAddress {
// public:
//     constexpr explicit GPUVirtualAddress(u32 address)
//         : m_address(address)
//     {
//     }
//
//     [[nodiscard]] constexpr u32 get() const { return m_address; }
//
// private:
//     u32 m_address { 0 };
// };

}

// template<>
// struct AK::Formatter<Kernel::RPi::V3D::GPUVirtualAddress> : Formatter<FormatString> {
//     ErrorOr<void> format(FormatBuilder& builder, Kernel::RPi::V3D::GPUVirtualAddress value)
//     {
//         return AK::Formatter<FormatString>::format(builder, "GV{}"sv, value.as_ptr());
//     }
// };
