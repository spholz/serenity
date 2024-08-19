/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Error.h>
#include <AK/NonnullOwnPtr.h>
#include <Kernel/Firmware/DeviceTree/DeviceTreeManagement.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>

namespace Kernel::DeviceTree {

// XXX: constexpr?
static auto const __compatibles = to_array<StringView const>({
    { "simple-bus"sv },
});

class SimpleBusDriver final : public DeviceTree::Driver {
public:
    SimpleBusDriver()
        : DeviceTree::Driver("SimpleBusDriver"sv)
    {
    }

    static void init();
    Span<StringView const> compatibles() override { return __compatibles; }
    ErrorOr<void> probe(DeviceTree::Device&) override;
};

void SimpleBusDriver::init()
{
    auto driver = MUST(adopt_nonnull_own_or_enomem(new (nothrow) SimpleBusDriver()));
    DeviceTree::Management::register_driver(move(driver));
}

ErrorOr<void> SimpleBusDriver::probe(DeviceTree::Device& device)
{
    DeviceTree::Management::the().scan_node_for_devices(device.node());
    return {};
}

DEVICETREE_DEVICE_DRIVER(SimpleBusDriver);

}
