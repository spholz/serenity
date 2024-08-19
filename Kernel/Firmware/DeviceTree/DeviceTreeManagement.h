/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// XXX: Rename to Management.h?

#include <AK/HashMap.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Libraries/LibDeviceTree/DeviceTree.h>

namespace Kernel::DeviceTree {

class Management {
public:
    static Management& the();

    static void register_driver(NonnullOwnPtr<DeviceTree::Driver>&&);

    void scan_node_for_devices(::DeviceTree::DeviceTreeNodeView const& node);

private:
    HashMap<StringView, NonnullOwnPtr<DeviceTree::Driver>> m_drivers;
    Vector<NonnullOwnPtr<DeviceTree::Device>> m_devices;
};

}
