/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Firmware/DeviceTree/Device.h>
#include <Kernel/Memory/TypedMapping.h>

struct V3DJob;

namespace Kernel::RPi::V3D {

struct HubRegisters;
struct CoreRegisters;
struct GPU3DDevice;

class V3D : public AtomicRefCounted<V3D> {
public:
    static ErrorOr<NonnullRefPtr<V3D>> create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource);

    void submit_job(V3DJob const&);

private:
    V3D(Memory::TypedMapping<HubRegisters volatile>, Memory::TypedMapping<CoreRegisters volatile>);

    ErrorOr<void> initialize();

    Memory::TypedMapping<HubRegisters volatile> m_hub_registers;
    Memory::TypedMapping<CoreRegisters volatile> m_core_0_registers;

    RefPtr<GPU3DDevice> m_3d_device;
};

}
