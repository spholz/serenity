/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/Storage/SD/Registers.h>
#include <Kernel/Devices/Storage/SD/SDHostController.h>
#include <Kernel/Devices/Storage/StorageManagement.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>
#include <Kernel/Memory/TypedMapping.h>

namespace Kernel {

class DeviceTreeSDHostController : public SDHostController {
public:
    DeviceTreeSDHostController(Memory::TypedMapping<SD::HostControlRegisterMap volatile>);
    virtual ~DeviceTreeSDHostController() override = default;

protected:
    // ^SDHostController
    virtual SD::HostControlRegisterMap volatile* get_register_map_base_address() override { return m_registers.ptr(); }
    // virtual ErrorOr<u32> retrieve_sd_clock_frequency() override;

private:
    Memory::TypedMapping<SD::HostControlRegisterMap volatile> m_registers;
};

DeviceTreeSDHostController::DeviceTreeSDHostController(Memory::TypedMapping<SD::HostControlRegisterMap volatile> registers)
    : m_registers(move(registers))
{
}

// ErrorOr<u32> DeviceTreeSDHostController::retrieve_sd_clock_frequency()
// {
//     return ENOTSUP;
// }

static constinit Array const compatibles_array = {
    "qcom,sdhci-msm-v5"sv,
};

DEVICETREE_DRIVER(DeviceTreeSDHCIController, compatibles_array);

// XXX: Devicetree binding reference
ErrorOr<void> DeviceTreeSDHCIController::probe(DeviceTree::Device const& device, StringView) const
{
    auto physical_address = TRY(device.get_resource(0)).paddr;

    auto registers = TRY(Memory::map_typed_writable<SD::HostControlRegisterMap volatile>(physical_address));
    auto sdhc = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) DeviceTreeSDHostController(move(registers))));
    TRY(sdhc->initialize());

    TRY(StorageManagement::the().add_controller(*sdhc));

    return {};
}

}
