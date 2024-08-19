/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Singleton.h>
#include <Kernel/Firmware/DeviceTree/DeviceTreeManagement.h>

namespace Kernel::DeviceTree {

static Singleton<Management> s_the;

Management& Management::the()
{
    return *s_the;
}

void Management::register_driver(NonnullOwnPtr<DeviceTree::Driver>&& driver)
{
    for (auto compatible : driver->compatibles()) {
        // XXX: Should we support multiple drivers with the same compatibles?
        VERIFY(!s_the->m_drivers.contains(compatible));

        s_the->m_drivers.try_set(compatible, move(driver)).release_value_but_fixme_should_propagate_errors();

        // XXX: Attach this driver to supported devices here as well
    }
}

void Management::scan_node_for_devices(::DeviceTree::DeviceTreeNodeView const& node)
{
    for (auto const& [child_name, child] : node.children()) {
        // XXX: Handle duplicate scans
        auto device = adopt_nonnull_own_or_enomem(new (nothrow) DeviceTree::Device(child, child_name)).release_value_but_fixme_should_propagate_errors();
        m_devices.try_append(move(device)).release_value_but_fixme_should_propagate_errors();

        auto const maybe_compatible = child.get_property("compatible"sv);
        if (!maybe_compatible.has_value())
            continue;

        DeviceTree::Driver* driver = nullptr;
        for (auto compatible_entry : maybe_compatible.value().as_strings()) {
            auto maybe_driver = m_drivers.get(compatible_entry);
            if (!maybe_driver.has_value())
                continue;

            driver = maybe_driver.release_value();
            break;
        }

        if (driver == nullptr)
            continue;

        if (auto result = driver->probe(*m_devices.last()); result.is_error())
            dbgln("DeviceTree: Failed to attach device {} to driver {} due to {}", child_name, driver->name(), result.release_error());
        else
            dbgln("DeviceTree: Attached device {} to driver {}", child_name, driver->name());
    }
}

}
