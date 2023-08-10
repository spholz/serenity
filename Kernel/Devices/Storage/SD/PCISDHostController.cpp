/*
 * Copyright (c) 2023, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Devices/Storage/SD/PCISDHostController.h>

namespace Kernel {

ErrorOr<NonnullRefPtr<PCISDHostController>> PCISDHostController::try_initialize(PCI::DeviceIdentifier const& device_identifier)
{
    auto sdhc = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) PCISDHostController(device_identifier)));
    TRY(sdhc->initialize());

    PCI::enable_bus_mastering(sdhc->device_identifier());
    PCI::enable_memory_space(sdhc->device_identifier());
    sdhc->try_enable_dma();

    return sdhc;
}

PCISDHostController::PCISDHostController(PCI::DeviceIdentifier const& device_identifier)
    : PCI::Device(device_identifier)
    , SDHostController()
{
    auto slot_information_register = read_slot_information();

    if (slot_information_register.slots_available() != 1) {
        // TODO: Support multiple slots
        dmesgln("SD Host Controller has {} slots, but we currently only support using only one", slot_information_register.slots_available());
    }

    VERIFY(slot_information_register.first_bar_number == 0);
    {
        SpinlockLocker locker(device_identifier.operation_lock());
        PCI::write32_locked(device_identifier, PCI::RegisterOffset::BAR0, 0x4000'0000);
        PCI::write32_locked(device_identifier, PCI::RegisterOffset::COMMAND, 0x6);
    }

    auto physical_address_of_sdhc_registers = PhysicalAddress {
        PCI::get_BAR(device_identifier, static_cast<PCI::HeaderType0BaseRegister>(slot_information_register.first_bar_number))
    };
    m_registers = Memory::map_typed_writable<SD::HostControlRegisterMap volatile>(physical_address_of_sdhc_registers).release_value_but_fixme_should_propagate_errors();
}

}
