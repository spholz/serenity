/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Bus/PCI/API.h>
#include <Kernel/Devices/Storage/NVMe/Properties.h>
#include <Kernel/Devices/Storage/NVMe/Queue.h>
#include <Kernel/Devices/Storage/StorageController.h>
#include <Kernel/Devices/Storage/StorageDevice.h>
#include <Kernel/Library/IOWindow.h>

namespace Kernel::NVMe {

class PCIeController final
    : public StorageController
    , public PCI::Device {
public:
    static ErrorOr<NonnullRefPtr<PCIeController>> create(PCI::DeviceIdentifier const&, bool nvme_poll);

    // ^StorageController
    virtual LockRefPtr<StorageDevice> device(u32 index) const override;
    virtual size_t devices_count() const override;

    // ^PCI::Device
    virtual StringView device_name() const override { return "NVMe::PCIeController"sv; }

protected:
    // ^StorageController
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

private:
    PCIeController(PCI::DeviceIdentifier const&, Memory::TypedMapping<u8 volatile> registers);

    void set_property32(PropertyOffset, u32 value);
    void set_property64(PropertyOffset, u64 value);
    u32 get_property32(PropertyOffset);
    u64 get_property64(PropertyOffset);

    template<typename Property>
    Property get_property()
    requires(requires { Property::PROPERTY_OFFSET; })
    {
        if constexpr (sizeof(Property) == sizeof(u32)) {
            return bit_cast<Property>(get_property32(Property::PROPERTY_OFFSET));
        } else if constexpr (sizeof(Property) == sizeof(u64)) {
            return bit_cast<Property>(get_property64(Property::PROPERTY_OFFSET));
        } else {
            VERIFY_NOT_REACHED();
        }
    }

    template<typename Property>
    void set_property(Property property)
    requires(requires { Property::PROPERTY_OFFSET; })
    {
        if constexpr (sizeof(Property) == sizeof(u32)) {
            return set_property32(Property::PROPERTY_OFFSET, bit_cast<u32>(property));
        } else if constexpr (sizeof(Property) == sizeof(u64)) {
            return set_property64(Property::PROPERTY_OFFSET, bit_cast<u64>(property));
        } else {
            VERIFY_NOT_REACHED();
        }
    }

    void ring_submission_queue_tail_doorbell(size_t queue_identifier, u32 new_tail_value);
    void ring_completion_queue_head_doorbell(size_t queue_identifier, u32 new_head_value);

    ErrorOr<void> admin_cmd_identify(Memory::ContiguousDMABuffer&, ControllerOrNamespaceStructure, u32 namespace_identifier = 0, Optional<CommandSetIdentifier> = {}, u16 cns_specific_identifier = 0, u16 controller_identifier = 0, u8 uuid_index = 0);

    ErrorOr<void> initialize();

    // XXX: Make TypedMapping requires volatile || const
    Memory::TypedMapping<u8 volatile> m_registers;

    Vector<LockRefPtr<StorageDevice>> m_storage_devices;

    OwnPtr<SubmissionQueue> m_admin_submission_queue;
    OwnPtr<CompletionQueue> m_admin_completion_queue;

    Optional<Memory::ContiguousDMABuffer> m_identify_dma_buffer;

    size_t m_doorbell_stride { 0 };
};

}
