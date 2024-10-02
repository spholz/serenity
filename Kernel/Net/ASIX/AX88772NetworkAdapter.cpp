/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/ByteBuffer.h>
#include <Kernel/Arch/Delay.h>
#include <Kernel/Bus/USB/USBEndpoint.h>
#include <Kernel/Bus/USB/USBManagement.h>
#include <Kernel/Bus/USB/USBRequest.h>
#include <Kernel/Net/ASIX/AX88772NetworkAdapter.h>
#include <Kernel/Net/NetworkingManagement.h>
#include <Kernel/Tasks/Process.h>

namespace Kernel::USB {

USB_DEVICE_DRIVER(AX88772NetworkAdapterDriver);

static constexpr u8 EMBEDDED_PHY_ID = 0b1'0000;

// 6.2
enum class Request : u8 {
    WriteSoftwareSerialManagementControlRegister = 0x06,
    ReadPHYRegister = 0x07,
    WritePHYRegister = 0x08,
    ReadSerialManagementStatusRegister = 0x09,
    WriteHardwareSerialManagementControlRegister = 0x0a,
    WriteRxControlRegister = 0x10,
    ReadNodeIDRegister = 0x13,
    WriteMediumModeRegister = 0x1b,
    WriteSoftwareResetRegister = 0x20,
    WriteSoftwarePHYSelectRegister = 0x22,
};

// 6.2.1.6
enum class SerialManagementStatusRegisterFlags : u8 {
    Host_EN = 1 << 0,
};
AK_ENUM_BITWISE_OPERATORS(SerialManagementStatusRegisterFlags)

// 6.2.1.12
enum class RxControlRegisterFlags : u16 {
    PRO = 1 << 0,
    AMALL = 1 << 1,
    SEP = 1 << 2,
    AB = 1 << 3,
    AM = 1 << 4,
    AP = 1 << 5,
    SO = 1 << 7,
};
AK_ENUM_BITWISE_OPERATORS(RxControlRegisterFlags)

static constexpr size_t RX_CONTROL_REGISTER_MFB_2048 = 0b00 << 8;
static constexpr size_t RX_CONTROL_REGISTER_MFB_4096 = 0b01 << 8;
static constexpr size_t RX_CONTROL_REGISTER_MFB_8192 = 0b10 << 8;
static constexpr size_t RX_CONTROL_REGISTER_MFB_16384 = 0b11 << 8;

// 6.2.1.18
enum class MediumStatusAndModeRegisterFlags : u16 {
    FD = 1 << 1,
    AlwaysOne = 1 << 2,
    RFC = 1 << 4,
    TFC = 1 << 5,
    PF = 1 << 7,
    RE = 1 << 8,
    PS = 1 << 9,
    SBP = 1 << 11,
    SM = 1 << 12,
};
AK_ENUM_BITWISE_OPERATORS(MediumStatusAndModeRegisterFlags)

// 6.2.1.23
enum class WriteSoftwareResetRegisterFlags : u8 {
    None = 0,

    RR = 1 << 0,
    RT = 1 << 1,
    PRTE = 1 << 2,
    PRL = 1 << 3,
    BZ = 1 << 4,
    IPRL = 1 << 5,
    IPPD = 1 << 6,
};
AK_ENUM_BITWISE_OPERATORS(WriteSoftwareResetRegisterFlags)

// The ASIX datasheet uses different names for the PHY registers, but they seem to match the 802.3 MII registers exactly.

// IEEE 802.3-2022 22.2.4
enum class MediaIndependentInterfaceRegisterAddress : u8 {
    Control = 0,
    Status = 1,
    PHYIdentifier1 = 2,
    PHYIdentifier2 = 3,
    AutoNegotiationAdvertisement = 4,
    AutoNegotiationLinkPartnerBasePageAbility = 5,
    AutoNegotiationExpansion = 6,
    AutoNegotiationNextPageTransmit = 7,
    AutoNegotiationLinkPartnerReceivedNextPage = 8,
    MasterSlaveControlRegister = 9,
    MasterSlaveStatusRegistert = 10,
    PSEControlRegister = 11,
    PSEStatusRegister = 12,
    MMDAccessControlRegister = 13,
    MMDAccessAddressDataRegister = 14,
    ExtendedStatus = 15,
};

// IEEE 802.3-2022 22.2.4.2
enum class MediaIndependentInterfaceStatusRegisterFlags : u8 {
    LinkStatusUp = 1 << 2,
};

// IEEE 802.3-2022 22.2.4.2
enum class MediaIndependentInterfaceLinkPartnerRegisterFlags : u8 {
    LinkStatusUp = 1 << 2,
};

void AX88772NetworkAdapterDriver::init()
{
    auto driver = MUST(adopt_nonnull_lock_ref_or_enomem(new AX88772NetworkAdapterDriver()));
    USBManagement::register_driver(driver);
}

ErrorOr<void> AX88772NetworkAdapterDriver::probe(Device& device)
{
    if (device.device_descriptor().vendor_id != 0x0b95      // ASIX Electronics Corporation
        || device.device_descriptor().product_id != 0x7720) // AX88772
        return ENOTSUP;

    // Section 5.0 of the AX88772 datasheet says the device supports 1 configuration and interface only.
    if (device.configurations().size() != 1)
        return ENOTSUP;

    auto const& configuration = device.configurations()[0];

    if (configuration.interface_count() != 1)
        return ENOTSUP;

    // FIXME: What about alternate settings?
    auto const& interface = configuration.interfaces()[0];

    dmesgln("AX88772: Found: {:04x}:{:04x}", device.device_descriptor().vendor_id, device.device_descriptor().product_id);

    TRY(device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_STANDARD | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        USB_REQUEST_SET_CONFIGURATION, configuration.configuration_id(), 0, 0, nullptr));

    USBEndpointDescriptor const* interrupt_endpoint_descriptor = nullptr;
    USBEndpointDescriptor const* bulk_in_endpoint_descriptor = nullptr;
    USBEndpointDescriptor const* bulk_out_endpoint_descriptor = nullptr;

    // 5.3 USB Endpoints
    for (auto const& endpoint_descriptor : interface.endpoints()) {
        if ((endpoint_descriptor.endpoint_address & 0b10001111) == 0x81 && (endpoint_descriptor.endpoint_attributes_bitmap & 0b11) == USBEndpoint::ENDPOINT_ATTRIBUTES_TRANSFER_TYPE_INTERRUPT)
            interrupt_endpoint_descriptor = &endpoint_descriptor;
        if ((endpoint_descriptor.endpoint_address & 0b10001111) == 0x82 && (endpoint_descriptor.endpoint_attributes_bitmap & 0b11) == USBEndpoint::ENDPOINT_ATTRIBUTES_TRANSFER_TYPE_BULK)
            bulk_in_endpoint_descriptor = &endpoint_descriptor;
        if ((endpoint_descriptor.endpoint_address & 0b10001111) == 0x03 && (endpoint_descriptor.endpoint_attributes_bitmap & 0b11) == USBEndpoint::ENDPOINT_ATTRIBUTES_TRANSFER_TYPE_BULK)
            bulk_out_endpoint_descriptor = &endpoint_descriptor;
    }

    if (interrupt_endpoint_descriptor == nullptr || bulk_in_endpoint_descriptor == nullptr || bulk_out_endpoint_descriptor == nullptr) {
        dmesgln("AX88772: Failed to find all required endpoint descriptors");
        return ENOTSUP;
    }

    if (interrupt_endpoint_descriptor->max_packet_size < 8) {
        dmesgln("AX88772: Unsupported interrupt endpoint max packet size: {}", interrupt_endpoint_descriptor->max_packet_size);
        return ENOTSUP;
    }

    if (bulk_in_endpoint_descriptor->max_packet_size < 4) {
        dmesgln("AX88772: Unsupported bulk in endpoint max packet size: {}", bulk_in_endpoint_descriptor->max_packet_size);
        return ENOTSUP;
    }

    if (bulk_out_endpoint_descriptor->max_packet_size < 4) {
        dmesgln("AX88772: Unsupported bulk out endpoint max packet size: {}", bulk_out_endpoint_descriptor->max_packet_size);
        return ENOTSUP;
    }

    auto interrupt_pipe = TRY(USB::InterruptInPipe::create(device.controller(), device, 1, interrupt_endpoint_descriptor->max_packet_size, 10));
    auto bulk_in_pipe = TRY(USB::BulkInPipe::create(device.controller(), device, 2, bulk_in_endpoint_descriptor->max_packet_size));
    auto bulk_out_pipe = TRY(USB::BulkOutPipe::create(device.controller(), device, 3, bulk_out_endpoint_descriptor->max_packet_size));

    auto send_buffer = TRY(FixedArray<u8>::create(bulk_out_pipe->max_packet_size()));

    auto adapter = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) AX88772NetworkAdapter(device, move(interrupt_pipe), move(bulk_in_pipe), move(bulk_out_pipe), move(send_buffer))));
    TRY(adapter->initialize());
    NetworkingManagement::the().add_adapter(adapter);

    return {};
}

void AX88772NetworkAdapterDriver::detach(Device&)
{
}

AX88772NetworkAdapter::AX88772NetworkAdapter(Device& device, NonnullOwnPtr<InterruptInPipe> interrupt_in_pipe, NonnullOwnPtr<BulkInPipe> bulk_in_pipe, NonnullOwnPtr<BulkOutPipe> bulk_out_pipe, FixedArray<u8> send_buffer)
    : NetworkAdapter("ethusbtodo"sv)
    , m_device(device)
    , m_interrupt_pipe(move(interrupt_in_pipe))
    , m_bulk_in_pipe(move(bulk_in_pipe))
    , m_bulk_out_pipe(move(bulk_out_pipe))
    , m_send_buffer(move(send_buffer))
{
}

ErrorOr<void> AX88772NetworkAdapter::initialize(Badge<NetworkingManagement>)
{
    // This driver shouldn't be initialized from NetworkingManagement
    VERIFY_NOT_REACHED();
}

ErrorOr<void> AX88772NetworkAdapter::initialize()
{
    MACAddress mac_address;
    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_DEVICE_TO_HOST,
        to_underlying(Request::ReadNodeIDRegister), 0, 0, sizeof(MACAddress), &mac_address));

    set_mac_address(mac_address);

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareResetRegister), to_underlying(WriteSoftwareResetRegisterFlags::IPRL | WriteSoftwareResetRegisterFlags::IPPD), 0, 0, nullptr));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareResetRegister), to_underlying(WriteSoftwareResetRegisterFlags::IPRL), 0, 0, nullptr));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareResetRegister), to_underlying(WriteSoftwareResetRegisterFlags::None), 0, 0, nullptr));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareResetRegister), to_underlying(WriteSoftwareResetRegisterFlags::IPRL), 0, 0, nullptr));

    for (u8 mii_reg_addr = 0; mii_reg_addr < 7; mii_reg_addr++)
        dbgln("PHY {:#x} MII reg {:#x}: {:#x}", EMBEDDED_PHY_ID, mii_reg_addr, MUST(read_phy_reg(EMBEDDED_PHY_ID, mii_reg_addr)));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteMediumModeRegister),
        to_underlying(MediumStatusAndModeRegisterFlags::FD | MediumStatusAndModeRegisterFlags::AlwaysOne | MediumStatusAndModeRegisterFlags::RE | MediumStatusAndModeRegisterFlags::PS),
        0, 0, nullptr));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteRxControlRegister),
        to_underlying(RxControlRegisterFlags::AB | RxControlRegisterFlags::AM | RxControlRegisterFlags::SO) | RX_CONTROL_REGISTER_MFB_16384,
        0, 0, nullptr));

    (void)TRY(m_interrupt_pipe->submit_interrupt_in_transfer(8, 10, [this](auto* transfer) {
        auto interrupt_frame = *bit_cast<u8 const*>(transfer->buffer().as_ptr());
        m_link_up = (interrupt_frame & 0b1) != 0;
    }));

    auto [process, thread] = TRY(Process::create_kernel_process("AX88772"sv, [this]() {
        auto max_chunk_size = m_bulk_out_pipe->max_packet_size();

        AK::Detail::ByteBuffer<8129> receive_buffer;

        for (;;) {
            receive_buffer.clear();

            size_t current_transfer_index = 0;

            receive_buffer.resize(current_transfer_index + max_chunk_size);
            auto bytes_received = m_bulk_in_pipe->submit_bulk_in_transfer(max_chunk_size, &receive_buffer[current_transfer_index]).release_value_but_fixme_should_propagate_errors();
            // dbgln("-> {:hex-dump}", ReadonlyBytes { &receive_buffer[current_transfer_index], bytes_received });
            current_transfer_index += bytes_received;

            auto payload_length = *bit_cast<LittleEndian<u16>*>(receive_buffer.data());
            auto payload_length_xor = *bit_cast<LittleEndian<u16>*>(&receive_buffer[2]);

            if ((payload_length ^ payload_length_xor) != 0xffff) {
                dbgln("invalid packet received");
                continue;
            }

            // TODO: Use Checked
            ssize_t remaining = payload_length - bytes_received - 4uz;

            while (remaining > 0) {
                receive_buffer.resize(current_transfer_index + max_chunk_size);
                auto bytes_received = m_bulk_in_pipe->submit_bulk_in_transfer(max_chunk_size, &receive_buffer[current_transfer_index]).release_value_but_fixme_should_propagate_errors();
                // dbgln("-> {:hex-dump}", ReadonlyBytes { &receive_buffer[current_transfer_index], bytes_received });
                current_transfer_index += bytes_received;

                remaining -= bytes_received;
            }

            // dbgln("did_receive {} Bytes, {:hex-dump}", payload_length, receive_buffer.span().slice(4, payload_length));
            did_receive(receive_buffer.span().slice(4, payload_length));
        }
    }));
    thread->set_name("AX88772"sv);

    return {};
}

i32 AX88772NetworkAdapter::link_speed()
{
    return 100; // TODO
}

bool AX88772NetworkAdapter::link_full_duplex()
{
    return true; // TODO
}

void AX88772NetworkAdapter::send_raw(ReadonlyBytes payload)
{
    auto max_chunk_size = m_bulk_out_pipe->max_packet_size();

    LittleEndian<u16> const length = payload.size();
    LittleEndian<u16> const length_xor = payload.size() ^ 0xffff;

    memcpy(m_send_buffer.data(), &length, sizeof(length));
    memcpy(&m_send_buffer[2], &length_xor, sizeof(length_xor));

    auto size_of_first_chunk = min(max_chunk_size - 4uz, payload.size());

    memcpy(&m_send_buffer[4], payload.data(), size_of_first_chunk);

    m_bulk_out_pipe->submit_bulk_out_transfer(size_of_first_chunk + 4uz, m_send_buffer.data()).release_value_but_fixme_should_propagate_errors();

    for (size_t i = size_of_first_chunk; i < payload.size(); i += max_chunk_size) {
        auto remaining = payload.size() - i;
        auto size_of_this_chunk = min(max_chunk_size, remaining);
        memcpy(m_send_buffer.data(), &payload[i], size_of_this_chunk);

        m_bulk_out_pipe->submit_bulk_out_transfer(size_of_this_chunk, m_send_buffer.data()).release_value_but_fixme_should_propagate_errors();
    }
}

ErrorOr<u16> AX88772NetworkAdapter::read_phy_reg(u8 phy_id, u8 address)
{
    VERIFY((phy_id & ~0x1f) == 0);
    VERIFY((address & ~0x1f) == 0);

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareSerialManagementControlRegister), 0, 0, 0, nullptr));

    u8 serial_management_status = 0;

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_DEVICE_TO_HOST,
        to_underlying(Request::ReadSerialManagementStatusRegister), 0, 0, sizeof(serial_management_status), &serial_management_status));

    if ((serial_management_status & to_underlying(SerialManagementStatusRegisterFlags::Host_EN)) == 0)
        return EIO;

    u16 value = 0;

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_DEVICE_TO_HOST,
        to_underlying(Request::ReadPHYRegister), phy_id, address, sizeof(value), &value));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteHardwareSerialManagementControlRegister), 0, 0, 0, nullptr));

    return value;
}

ErrorOr<void> AX88772NetworkAdapter::write_phy_reg(u8 phy_id, u8 address, u16 value)
{
    VERIFY((phy_id & ~0x1f) == 0);
    VERIFY((address & ~0x1f) == 0);

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteSoftwareSerialManagementControlRegister), 0, 0, 0, nullptr));

    u8 serial_management_status = 0;

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_DEVICE_TO_HOST,
        to_underlying(Request::ReadSerialManagementStatusRegister), 0, 0, sizeof(serial_management_status), &serial_management_status));

    if ((serial_management_status & to_underlying(SerialManagementStatusRegisterFlags::Host_EN)) == 0)
        return EIO;

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WritePHYRegister), phy_id, address, sizeof(value), &value));

    TRY(m_device.control_transfer(USB_REQUEST_RECIPIENT_DEVICE | USB_REQUEST_TYPE_VENDOR | USB_REQUEST_TRANSFER_DIRECTION_HOST_TO_DEVICE,
        to_underlying(Request::WriteHardwareSerialManagementControlRegister), 0, 0, 0, nullptr));

    return {};
}

}
