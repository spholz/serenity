/*
 * Copyright (c) 2024, Leon Albrecht <leon.a@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "DeviceTree.h"
#include <AK/Singleton.h>
#include <Kernel/Memory/MemoryManager.h>
#include <LibDeviceTree/DeviceTree.h>
#include <Userland/Libraries/LibDeviceTree/FlattenedDeviceTree.h>
#include <Userland/Libraries/LibDeviceTree/Validation.h>

#if ARCH(RISCV64)
#    include <Kernel/Arch/riscv64/CPU.h>
#endif

static Singleton<OwnPtr<DeviceTree::DeviceTree>> s_device_tree;

namespace Kernel::DeviceTree {

alignas(PAGE_SIZE) __attribute__((section(".bss.fdt"))) u8 s_fdt_storage[fdt_storage_size];

ErrorOr<void> unflatten_fdt()
{
    *s_device_tree = TRY(::DeviceTree::DeviceTree::parse({ s_fdt_storage, fdt_storage_size }));
    return {};
}

bool verify_fdt()
{
    static bool verified { false };
    static bool verification_succeeded { false };

    if (verified)
        return verification_succeeded;

    verified = true;
    auto& header = *bit_cast<::DeviceTree::FlattenedDeviceTreeHeader*>(&s_fdt_storage[0]);
    auto fdt = ReadonlyBytes(s_fdt_storage, header.totalsize);

    verification_succeeded = ::DeviceTree::validate_flattened_device_tree(header, fdt, ::DeviceTree::Verbose::No);

    return verification_succeeded;
}

void dump_fdt()
{
    auto& header = *bit_cast<::DeviceTree::FlattenedDeviceTreeHeader*>(&s_fdt_storage[0]);
    auto fdt = ReadonlyBytes(s_fdt_storage, header.totalsize);
    MUST(::DeviceTree::dump(header, fdt));
}

ErrorOr<StringView> get_command_line_from_fdt()
{
    auto& header = *bit_cast<::DeviceTree::FlattenedDeviceTreeHeader*>(&s_fdt_storage[0]);
    auto fdt = ReadonlyBytes(s_fdt_storage, header.totalsize);
    return TRY(::DeviceTree::slow_get_property("/chosen/bootargs"sv, header, fdt)).as_string();
}

::DeviceTree::DeviceTree const& get()
{
    VERIFY(*s_device_tree);
    return **s_device_tree;
}

}

#if ARCH(RISCV64)
namespace Kernel {

bool is_vf2()
{
    static TriState is_vf2 = TriState::Unknown;

    if (is_vf2 == TriState::Unknown) {
        auto& fdt_header = *bit_cast<::DeviceTree::FlattenedDeviceTreeHeader*>(&DeviceTree::s_fdt_storage[0]);
        auto fdt = ReadonlyBytes(DeviceTree::s_fdt_storage, fdt_header.totalsize);
        auto compatible = MUST(::DeviceTree::slow_get_property("/compatible"sv, fdt_header, fdt)).as_string();

        is_vf2 = compatible.starts_with("starfive,jh7110"sv) ? TriState::True : TriState::False;
    }

    if (is_vf2 == TriState::True)
        return true;

    return false;
}

}
#endif
