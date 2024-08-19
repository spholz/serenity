/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Noncopyable.h>
#include <LibDeviceTree/DeviceTree.h>

namespace Kernel::DeviceTree {

class Device {
    AK_MAKE_NONCOPYABLE(Device);

public:
    Device(::DeviceTree::DeviceTreeNodeView const& node, StringView node_name)
        : m_node(node)
        , m_node_name(node_name)
    {
    }

    ::DeviceTree::DeviceTreeNodeView const& node() { return m_node; }
    StringView node_name() { return m_node_name; }

private:
    ::DeviceTree::DeviceTreeNodeView const& m_node;
    StringView m_node_name;
};

}
