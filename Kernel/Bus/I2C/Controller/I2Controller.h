/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Types.h>

// I2C spec: https://www.nxp.com/docs/en/user-guide/UM10204.pdf

namespace Kernel::I2C {

// Struct representing a target-transmitter to controller-receiver transfer
struct ReadTransfer {
    u16 target_address; // 7-bit or 10-bit I2C target address
    Bytes data_read;
};

// Struct representing a controller-transmitter to target-receiver transfer
struct WriteTransfer {
    u16 target_address; // 7-bit or 10-bit I2C target address
    ReadonlyBytes data_to_write;
};

using Transfer = Variant<ReadTransfer, WriteTransfer>;

class I2CController {
public:
    virtual ~I2CController() = default;

    virtual ErrorOr<void> do_transfers(Span<Transfer>) = 0;

protected:
    I2CController() = default;
};

}
