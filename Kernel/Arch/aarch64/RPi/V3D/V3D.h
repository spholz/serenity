/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Firmware/DeviceTree/Device.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel::RPi::V3D {

struct HubRegisters;
struct CoreRegisters;

class V3D : public AtomicRefCounted<V3D> {
public:
    static ErrorOr<NonnullRefPtr<V3D>> create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource);

private:
    V3D(Memory::TypedMapping<HubRegisters volatile>, Memory::TypedMapping<CoreRegisters volatile>);

    ErrorOr<void> initialize();

    Memory::TypedMapping<HubRegisters volatile> m_hub_registers;
    Memory::TypedMapping<CoreRegisters volatile> m_core_0_registers;
};

}
