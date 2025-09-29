/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/aarch64/IRQController.h>
#include <Kernel/Firmware/DeviceTree/Device.h>
#include <Kernel/Memory/TypedMapping.h>

// GIC v3/v4 Architecture Specification: https://documentation-service.arm.com/static/661e85ca5d66282bc2cf9cc8
// Learn the architecture - Generic Interrupt Controller v3 and v4, Overview: https://documentation-service.arm.com/static/6645de4b4072745e25d819ee
// Learn the architecture - Generic Interrupt Controller v3 and v4, LPIs: https://documentation-service.arm.com/static/65ba2901c2052a35156cc629

// XXX: Doesn't need a header

namespace Kernel {

class GICv3 final : public IRQController {
public:
    static ErrorOr<NonnullLockRefPtr<GICv3>> try_to_initialize(DeviceTree::Device::Resource distributor_registers_resource, DeviceTree::Device::Resource redistributor_registers_registers_resource);

    virtual void enable(GenericInterruptHandler const&) override;
    virtual void disable(GenericInterruptHandler const&) override;

    virtual void eoi(GenericInterruptHandler const&) override;

    virtual Optional<size_t> pending_interrupt() const override;

    virtual StringView model() const override { return "GICv3"sv; }

    struct DistributorRegisters;
    struct RedistributorRegisters;

private:
    GICv3(Memory::TypedMapping<DistributorRegisters volatile>, Memory::TypedMapping<RedistributorRegisters volatile>);

    ErrorOr<void> initialize();

    Memory::TypedMapping<DistributorRegisters volatile> m_distributor_registers;
    Memory::TypedMapping<RedistributorRegisters volatile> m_redistributor_registers;
};

}
