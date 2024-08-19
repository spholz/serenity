/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/AtomicRefCounted.h>
#include <AK/StringView.h>
#include <Kernel/Firmware/DeviceTree/Device.h>

namespace Kernel::DeviceTree {

using DriverInitFunction = void (*)();
#define DEVICETREE_DEVICE_DRIVER(driver_name) \
    DriverInitFunction driver_init_function_ptr_##driver_name __attribute__((section(".driver_init"), used)) = &driver_name::init

class Driver {
    AK_MAKE_NONCOPYABLE(Driver);
    AK_MAKE_NONMOVABLE(Driver);

public:
    Driver(StringView name)
        : m_driver_name(name)
    {
    }

    virtual ~Driver() = default;
    virtual Span<StringView const> compatibles() = 0;
    virtual ErrorOr<void> probe(DeviceTree::Device&) = 0;

    StringView name() const { return m_driver_name; }

private:
    StringView const m_driver_name;
};

}
