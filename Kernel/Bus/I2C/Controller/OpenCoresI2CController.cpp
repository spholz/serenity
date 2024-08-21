/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Bus/I2C/Controller/OpenCoresI2CController.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>

namespace Kernel::I2C {

ErrorOr<NonnullOwnPtr<OpenCoresI2CController>> OpenCoresI2CController::try_to_initialize()
{
    auto const maybe_soc_node = DeviceTree::get().get_child("soc"sv);
    if (!maybe_soc_node.has_value()) {
        return ENODEV;
    }
    auto const& soc_node = maybe_soc_node.value();
    auto soc_address_cells = soc_node.get_property("#address-cells"sv).value().as<u32>();

    for (auto const& [node_name, node] : soc_node.children()) {
        if (!node_name.starts_with("i2c"sv))
            continue;

        auto maybe_compatible = node.get_property("compatible"sv);
        if (!maybe_compatible.has_value())
            continue;
        auto compatible = maybe_compatible.value();

        // FIXME: Support more than one controller.
        if (!compatible.as_strings().contains_slow("opencores,i2c-ocores"sv))
            continue;

        u32 register_shift = 0;
        auto maybe_register_shift = node.get_property("reg-shift"sv);
        if (maybe_register_shift.has_value())
            register_shift = maybe_register_shift.value().as<u32>();

        u32 register_io_width = 1;
        auto maybe_register_io_width = node.get_property("reg-io-width"sv);
        if (maybe_register_io_width.has_value())
            register_io_width = maybe_register_io_width.value().as<u32>();

        if (register_io_width != 1 && register_io_width != 2 && register_io_width != 4)
            return ENOTSUP;

        auto maybe_reg = node.get_property("reg"sv);
        if (!maybe_reg.has_value())
            return ENOTSUP;

        auto reg_stream = maybe_reg.value().as_stream();

        PhysicalAddress paddr;
        if (soc_address_cells == 1)
            paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u32>>()) };
        else if (soc_address_cells == 2)
            paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u64>>()) };
        else
            return ENOTSUP;

        if (paddr.get() % register_io_width != 0)
            return ENOTSUP;

        auto register_region_size = TRY(Memory::page_round_up(paddr.offset_in_page() + REGISTER_COUNT * (1 << register_shift)));
        auto register_region = TRY(MM.allocate_mmio_kernel_region(paddr.page_base(), register_region_size, {}, Memory::Region::Access::ReadWrite));
        auto register_address = register_region->vaddr().offset(paddr.offset_in_page());

        return TRY(adopt_nonnull_own_or_enomem(new (nothrow) OpenCoresI2CController(move(register_region), register_address, register_shift, register_io_width)));
    }

    return ENODEV;
}

OpenCoresI2CController::OpenCoresI2CController(NonnullOwnPtr<Memory::Region> register_region, VirtualAddress register_address, u32 register_shift, u32 register_io_width)
    : m_register_region(move(register_region))
    , m_register_address(register_address)
    , m_register_shift(register_shift)
    , m_register_io_width(register_io_width)
{
    // Set the core to be enabled and interrupts to be disabled.
    write_reg(RegisterOffset::CTR, to_underlying(ControlRegisterFlags::EN));
}

ErrorOr<void> OpenCoresI2CController::do_transfers(Span<Transfer> transfers)
{
    for (size_t transfer_index = 0; transfer_index < transfers.size(); transfer_index++) {
        TRY(transfers[transfer_index].visit(
            [&](ReadTransfer& transfer) -> ErrorOr<void> {
                // FIXME: Support 10-bit addresses
                if (transfer.target_address > 0x7f)
                    return ENOTSUP;

                dbgln("read start");

                // Send the address.
                // The least significant bit being 1 means read.
                write_reg(RegisterOffset::TXR, (transfer.target_address << 1) | 0b1);
                write_reg(RegisterOffset::CR, to_underlying(CommandRegisterFlags::WR | CommandRegisterFlags::STA));

                dbgln("read data");

                for (size_t byte_index = 0; byte_index < transfer.data_read.size(); byte_index++) {
                    if ((read_reg(RegisterOffset::SR) & to_underlying(StatusRegisterFlags::RxACK)) != 0) {
                        write_reg(RegisterOffset::CR, to_underlying(CommandRegisterFlags::STO));
                        return EIO;
                    }

                    transfer.data_read[byte_index] = read_reg(RegisterOffset::RXR);

                    bool is_last_byte_of_read_transfer = (byte_index == transfer.data_read.size() - 1);
                    bool is_last_byte_to_be_transferred = is_last_byte_of_read_transfer && (transfer_index == transfers.size() - 1);

                    auto command_reg_flags = CommandRegisterFlags::RD;
                    if (is_last_byte_of_read_transfer)
                        command_reg_flags |= CommandRegisterFlags::ACK; // ACK = 1 means 'NACK'
                    if (is_last_byte_to_be_transferred)
                        command_reg_flags |= CommandRegisterFlags::STO;

                    write_reg(RegisterOffset::CR, to_underlying(command_reg_flags));
                }

                dbgln("read done");

                return {};
            },
            [&](WriteTransfer const& transfer) -> ErrorOr<void> {
                // FIXME: Support 10-bit addresses
                if (transfer.target_address > 0x7f)
                    return ENOTSUP;

                dbgln("write start");

                // Send the address.
                // The least significant bit being 0 means write.
                write_reg(RegisterOffset::TXR, transfer.target_address << 1);
                write_reg(RegisterOffset::CR, to_underlying(CommandRegisterFlags::WR | CommandRegisterFlags::STA));

                dbgln("write data");

                for (size_t byte_index = 0; byte_index < transfer.data_to_write.size(); byte_index++) {
                    if ((read_reg(RegisterOffset::SR) & to_underlying(StatusRegisterFlags::RxACK)) != 0) {
                        write_reg(RegisterOffset::CR, to_underlying(CommandRegisterFlags::STO));
                        return EIO;
                    }

                    write_reg(RegisterOffset::TXR, transfer.data_to_write[byte_index]);

                    bool is_last_byte_to_be_transferred = (byte_index == transfer.data_to_write.size() - 1) && (transfer_index == transfers.size() - 1);

                    auto command_reg_flags = CommandRegisterFlags::WR;
                    if (is_last_byte_to_be_transferred)
                        command_reg_flags |= CommandRegisterFlags::STO;

                    write_reg(RegisterOffset::CR, to_underlying(command_reg_flags));
                }

                dbgln("write done");

                return {};
            }));
    }

    return {};
}

}
