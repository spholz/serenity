/*
 * Copyright (c) 2024, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Badge.h>
#include <AK/IntrusiveList.h>
#include <AK/Noncopyable.h>
#include <Kernel/Memory/PhysicalAddress.h>
#include <Kernel/Memory/Region.h>
#include <LibDeviceTree/DeviceTree.h>

namespace Kernel::DeviceTree {

class Driver;
class Management;

class ContiguousDMABuffer {
public:
    u64 bus_address() const;
    VirtualAddress virtual_address() const;

private:
    NonnullOwnPtr<Memory::Region> m_region;
};

class Device {
    AK_MAKE_NONCOPYABLE(Device);
    AK_MAKE_NONMOVABLE(Device);

public:
    Device(::DeviceTree::Node const& node, StringView node_name)
        : m_node(node)
        , m_node_name(node_name)
    {
    }

    ::DeviceTree::Node const& node() const { return m_node; }
    StringView node_name() const { return m_node_name; }

    Driver const* driver() const { return m_driver; }
    void set_driver(Badge<Management>, Driver const& driver)
    {
        VERIFY(m_driver == nullptr);
        m_driver = &driver;
    }

    struct Resource {
        PhysicalAddress paddr;
        size_t size;
    };
    ErrorOr<Resource> get_resource(size_t index) const;

    bool is_dma_cache_coherent() const;

    ErrorOr<ContiguousDMABuffer> allocate_contiguous_dma_buffer(StringView name, Memory::Region::Access access, size_t size) const;

    // FIXME: Add support for the "interrupt-names" property to resolve interrupts by name.
    ErrorOr<size_t> get_interrupt_number(size_t index) const;

private:
    ::DeviceTree::Node const& m_node;
    StringView m_node_name;
    Driver const* m_driver { nullptr };

    IntrusiveListNode<Device> m_list_node;

public:
    using List = IntrusiveList<&Device::m_list_node>;
};

}
