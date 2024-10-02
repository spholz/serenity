/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

// AX88772 datasheet: https://web.archive.org/web/20061009143750/http://asix.com.tw/FrootAttach/datasheet/AX88772_datasheet_Rev10.pdf

#include <Kernel/Bus/USB/Drivers/USBDriver.h>
#include <Kernel/Bus/USB/USBPipe.h>
#include <Kernel/Net/NetworkAdapter.h>

namespace Kernel::USB {

// Move this to USB/Drivers?
class AX88772NetworkAdapterDriver final : public Driver {
public:
    AX88772NetworkAdapterDriver()
        : Driver("AX8872 Network Adapter"sv)
    {
    }

    static void init();

    ~AX88772NetworkAdapterDriver() override = default;

    ErrorOr<void> probe(USB::Device&) override;
    void detach(USB::Device&) override;
};

class AX88772NetworkAdapter final : public NetworkAdapter {
public:
    AX88772NetworkAdapter(Device&, NonnullOwnPtr<InterruptInPipe>, NonnullOwnPtr<BulkInPipe>, NonnullOwnPtr<BulkOutPipe>, FixedArray<u8> send_buffer);

    StringView class_name() const override
    {
        return "AX88772NetworkAdapter"sv;
    }

    Type adapter_type() const override { return Type::Ethernet; }

    ErrorOr<void> initialize(Badge<NetworkingManagement>) override;
    ErrorOr<void> initialize();

    bool link_up() override { return m_link_up; }

    i32 link_speed() override;
    bool link_full_duplex() override;

protected:
    void send_raw(ReadonlyBytes) override;

private:
    ErrorOr<u16> read_phy_reg(u8 phy_id, u8 address);
    ErrorOr<void> write_phy_reg(u8 phy_id, u8 address, u16 value);

    Device& m_device;
    NonnullOwnPtr<InterruptInPipe> m_interrupt_pipe;
    NonnullOwnPtr<BulkInPipe> m_bulk_in_pipe;
    NonnullOwnPtr<BulkOutPipe> m_bulk_out_pipe;

    FixedArray<u8> m_send_buffer;

    bool m_link_up { false };
};

}
