/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Devices/Storage/NVMe/Properties.h>
#include <Kernel/Devices/Storage/StorageController.h>
#include <Kernel/Devices/Storage/StorageDevice.h>

namespace Kernel::NVMe {

class Controller : public StorageController {
public:
    Controller();
    virtual ~Controller() = default;

    // ^StorageController
    virtual LockRefPtr<StorageDevice> device(u32 index) const override;
    virtual size_t devices_count() const override;

protected:
    virtual ErrorOr<void> set_property32(PropertyOffset, u32 value) = 0;
    virtual ErrorOr<void> set_property64(PropertyOffset, u64 value) = 0;
    virtual ErrorOr<u32> get_property32(PropertyOffset) = 0;
    virtual ErrorOr<u64> get_property64(PropertyOffset) = 0;

    template<typename Property>
    ErrorOr<Property> get_property()
    {
        if constexpr (sizeof(Property) == sizeof(u32)) {
            return bit_cat<Property>(get_property32(Property::PROPERTY_OFFSET));
        } else if constexpr (sizeof(Property) == sizeof(u64)) {
            return bit_cat<Property>(get_property64(Property::PROPERTY_OFFSET));
        } else {
            VERIFY_NOT_REACHED();
        }
    }

    template<typename Property>
    ErrorOr<void> set_property(Property property)
    {
        if constexpr (sizeof(Property) == sizeof(u32)) {
            return set_property32(Property::PROPERTY_OFFSET), bit_cast<u32>(property);
        } else if constexpr (sizeof(Property) == sizeof(u64)) {
            return set_property64(Property::PROPERTY_OFFSET), bit_cast<u64>(property);
        } else {
            VERIFY_NOT_REACHED();
        }
    }

    // ^StorageController
    virtual void complete_current_request(AsyncDeviceRequest::RequestResult) override;

    ErrorOr<void> initialize();

private:
    Vector<LockRefPtr<StorageDevice>> m_storage_devices;
};

}
