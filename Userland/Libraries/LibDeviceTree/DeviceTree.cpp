/*
 * Copyright (c) 2024, Leon Albrecht <leon.a@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DeviceTree.h"
#include "FlattenedDeviceTree.h"
#include <AK/NonnullOwnPtr.h>
#include <AK/Types.h>

namespace DeviceTree {

ErrorOr<NonnullOwnPtr<DeviceTree>> DeviceTree::parse(ReadonlyBytes flattened_device_tree)
{
    // Device tree must be 8-byte aligned
    if ((bit_cast<FlatPtr>(flattened_device_tree.data()) & 0b111) != 0)
        return Error::from_errno(EINVAL);

    auto device_tree = TRY(adopt_nonnull_own_or_enomem(new (nothrow) DeviceTree { flattened_device_tree }));
    DeviceTreeNodeView* current_node = device_tree.ptr();

    auto const& header = *reinterpret_cast<FlattenedDeviceTreeHeader const*>(flattened_device_tree.data());

    TRY(walk_device_tree(header, flattened_device_tree,
        {
            .on_node_begin = [&current_node, &device_tree](StringView name) -> ErrorOr<IterationDecision> {
                // Skip the root node, which has an empty name
                if (current_node == device_tree.ptr() && name.is_empty())
                    return IterationDecision::Continue;

                // FIXME: Use something like children.emplace
                TRY(current_node->children().try_set(name, DeviceTreeNodeView { current_node }));
                auto& new_node = current_node->children().get(name).value();
                current_node = &new_node;
                return IterationDecision::Continue;
            },
            .on_node_end = [&current_node](StringView) -> ErrorOr<IterationDecision> {
                current_node = current_node->parent();
                return IterationDecision::Continue;
            },
            .on_property = [&current_node](StringView name, ReadonlyBytes value) -> ErrorOr<IterationDecision> {
                TRY(current_node->properties().try_set(name, DeviceTreeProperty { value }));
                return IterationDecision::Continue;
            },
            .on_noop = []() -> ErrorOr<IterationDecision> {
                return IterationDecision::Continue;
            },
            .on_end = [&]() -> ErrorOr<void> {
                return {};
            },
        }));

    // FIXME: While growing the a nodes children map, we might have reallocated it's storage
    //        breaking the parent pointers of the children, so we need to fix them here
    auto fix_parent = [](auto self, DeviceTreeNodeView& node) -> void {
        for (auto& [name, child] : node.children()) {
            child.m_parent = &node;
            self(self, child);
        }
    };

    fix_parent(fix_parent, *device_tree);
    // Note: For the same reason as above, we need to postpone setting the phandles until the tree is fully built
    TRY(device_tree->for_each_node([&device_tree]([[maybe_unused]] StringView name, DeviceTreeNodeView& node) -> ErrorOr<RecursionDecision> {
        if (auto phandle = node.get_property("phandle"sv); phandle.has_value()) {
            auto phandle_value = phandle.value().as<u32>();
            TRY(device_tree->set_phandle(phandle_value, &node));
        }
        return RecursionDecision::Recurse;
    }));

    return device_tree;
}

ErrorOr<DeviceTreeNodeView const*> DeviceTreeNodeView::interrupt_parent(DeviceTree const& device_tree) const
{
    auto maybe_interrupt_parent_prop = get_property("interrupt-parent"sv);
    if (maybe_interrupt_parent_prop.has_value()) {
        auto interrupt_parent_prop = maybe_interrupt_parent_prop.release_value();

        if (interrupt_parent_prop.size() != sizeof(Phandle))
            return EINVAL;

        auto const* interrupt_parent = device_tree.phandle(interrupt_parent_prop.as<u32>());

        if (interrupt_parent == nullptr)
            return ENOENT;

        return interrupt_parent;
    }

    if (m_parent == nullptr)
        return ENOENT;

    return m_parent;
}

ErrorOr<DeviceTreeNodeView const*> DeviceTreeNodeView::interrupt_domain_root(DeviceTree const& device_tree) const
{
    auto const* current_node = this;

    for (;;) {
        if (current_node->has_property("interrupt-controller"sv) || current_node->has_property("interrupt-map"sv))
            return current_node;

        current_node = TRY(current_node->interrupt_parent(device_tree));

        if (current_node == nullptr)
            return ENOENT;
    }
}

ErrorOr<FixedArray<Interrupt>> DeviceTreeNodeView::interrupts(DeviceTree const& device_tree) const
{
    auto const& domain_root = *TRY(interrupt_domain_root(device_tree));

    auto interrupt_cells_prop = domain_root.get_property("#interrupt-cells"sv);
    if (!interrupt_cells_prop.has_value())
        return EINVAL;

    if (interrupt_cells_prop->size() != sizeof(Cell))
        return EINVAL;

    auto interrupt_cells = interrupt_cells_prop->as<u32>();

    // XXX: TODO: interrupts-extended

    auto interrupts_prop = get_property("interrupts"sv);
    if (!interrupts_prop.has_value())
        return EINVAL;

    auto interrupts_raw = interrupts_prop->raw_data;

    // if (!domain_root.has_property("interrupt-controller"sv))
    //     return ENOTSUP; // TODO: Handle interrupt nexuses

    auto interrupt_count = interrupts_prop->size() / (interrupt_cells * sizeof(Cell));

    auto interrupts = TRY(FixedArray<Interrupt>::create(interrupt_count));

    for (size_t i = 0; i < interrupt_count; i++) {
        interrupts[i] = Interrupt {
            .domain_root = &domain_root,
            .interrupt_identifier = interrupts_raw.slice(i * interrupt_cells * sizeof(u32), interrupt_cells * sizeof(u32)),
        };
    }

    return interrupts;
}

bool DeviceTreeNodeView::is_compatible_with(StringView wanted_compatible) const
{
    auto maybe_compatible = get_property("compatible"sv);
    if (!maybe_compatible.has_value())
        return false;

    bool is_compatible = false;

    maybe_compatible->for_each_string([&is_compatible, wanted_compatible](StringView compatible_entry) {
        if (compatible_entry == wanted_compatible) {
            is_compatible = true;
            return IterationDecision::Break;
        }

        return IterationDecision::Continue;
    });

    return is_compatible;
}

ErrorOr<Reg> DeviceTreeNodeView::reg() const
{
    if (parent() == nullptr)
        return Error::from_errno(EINVAL);

    auto reg_prop = get_property("reg"sv);
    if (!reg_prop.has_value())
        return Error::from_errno(ENOENT);

    // If missing, a client program should assume a default value of 2 for #address-cells, and a value of 1 for #size-cells.
    auto parent_address_cells = parent()->address_cells().value_or(2);
    auto parent_size_cells = parent()->size_cells().value_or(1);

    Vector<RegEntry, 2> reg_entries;

    for (size_t i = 0; i < reg_prop->size(); i += (parent_address_cells + parent_size_cells) * sizeof(u32))
        TRY(reg_entries.try_empend(reg_prop->raw_data.slice(i), reg_prop->raw_data.slice(i + parent_address_cells)));

    return Reg { reg_entries };
}

}
