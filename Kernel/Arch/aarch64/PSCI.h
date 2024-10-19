/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <AK/Types.h>

// https://developer.arm.com/documentation/den0022/latest/

namespace Kernel::PSCI {

// 5.2.2 Return error codes
enum class ReturnErrorCode : u32 {
    SUCCESS = 0,
    NOT_SUPPORTED = bit_cast<u32>(-1),
    INVALID_PARAMETERS = bit_cast<u32>(-2),
    DENIED = bit_cast<u32>(-3),
    ALREADY_ON = bit_cast<u32>(-4),
    ON_PENDING = bit_cast<u32>(-5),
    INTERNAL_FAILURE = bit_cast<u32>(-6),
    NOT_PRESENT = bit_cast<u32>(-7),
    DISABLED = bit_cast<u32>(-8),
    INVALID_ADDRESS = bit_cast<u32>(-9),
};

template<typename T>
using ReturnErrorCodeOr = ErrorOr<T, ReturnErrorCode>;

// 5.3 PSCI_VERSION
struct Version {
    u32 minor : 16;
    u32 major : 16;
};
Version get_version();

// 5.16 PSCI_FEATURES
ReturnErrorCodeOr<u32> get_features(u32 psci_func_id);

}

template<>
struct AK::Formatter<Kernel::PSCI::Version> : Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, Kernel::PSCI::Version const& version)
    {
        return Formatter<FormatString>::format(builder, "{}.{}"sv, version.major, version.minor);
    }
};
