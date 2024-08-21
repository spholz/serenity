/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Bus/I2C/Controller/I2Controller.h>
#include <Kernel/Memory/MemoryManager.h>

// https://opencores.org/websvn/filedetails?repname=i2c&path=%2Fi2c%2Ftags%2Frel_1%2Fdoc%2Fi2c_specs.pdf

namespace Kernel::I2C {

class OpenCoresI2CController final : public I2CController {
public:
    static ErrorOr<NonnullOwnPtr<OpenCoresI2CController>> try_to_initialize();
    ~OpenCoresI2CController() override = default;

    ErrorOr<void> do_transfers(Span<Transfer>) override;

private:
    OpenCoresI2CController(NonnullOwnPtr<Memory::Region> register_region, VirtualAddress register_address, u32 register_shift, u32 register_io_width);

    NonnullOwnPtr<Memory::Region> m_register_region;
    VirtualAddress m_register_address;

    u32 m_register_shift;
    u32 m_register_io_width;

    // NOTE: Reserved bits should be filled with zeroes.

    enum class RegisterOffset {
        // Clock Prescale register lo-byte (RW)
        PRERlo = 0x00,

        // Clock Prescale register hi-byte (RW)
        PRERhi = 0x01,

        // Control register (RW)
        CTR = 0x02,

        // Transmit register (W)
        TXR = 0x03,

        // Receive register (R)
        RXR = 0x03,

        // Command register (W)
        CR = 0x04,

        // Status register (R)
        SR = 0x04,
    };

    static constexpr size_t REGISTER_COUNT = 5;

    enum class ControlRegisterFlags {
        // I2C core interrupt enable bit.
        // When set to ‘1’, interrupt is enabled.
        // When set to ‘0’, interrupt is disabled.
        IEN = 1 << 6,

        // I2C core enable bit.
        // When set to ‘1’, the core is enabled.
        // When set to ‘0’, the core is disabled
        EN = 1 << 7,
    };
    AK_ENUM_BITWISE_FRIEND_OPERATORS(ControlRegisterFlags);

    enum class CommandRegisterFlags {
        // Interrupt acknowledge. When set, clears a pending interrupt.
        IACK = 1 << 0,

        // When a receiver, sent ACK (ACK = ‘0’) or NACK (ACK = ‘1’)
        ACK = 1 << 3,

        // Write to slave
        WR = 1 << 4,

        // Read from slave
        RD = 1 << 5,

        // Generate stop condition
        STO = 1 << 6,

        // Generate (repeated) start condition
        STA = 1 << 7,
    };
    AK_ENUM_BITWISE_FRIEND_OPERATORS(CommandRegisterFlags);

    enum class StatusRegisterFlags {
        // Interrupt Flag. This bit is set when an interrupt is pending, which will cause a processor interrupt request if the IEN bit is set.
        // The Interrupt Flag is set when:
        // • one byte transfer has been completed
        // • arbitration is lost
        IF = 1 << 0,

        // Transfer in progress
        // ‘1’ when transferring data
        // ‘0’ when transfer complete
        TIP = 1 << 1,

        // Arbitration lost
        // This bit is set when the core lost arbitration. Arbitration is lost when:
        // • a STOP signal is detected, but non requested
        // • The master drives SDA high, but SDA is low
        AL = 1 << 5,

        // I2C bus busy
        // ‘1’ after START signal detected
        // ‘0’ after STOP signal detected
        Busy = 1 << 6,

        // Received acknowledge from slave
        // ‘1’ = No acknowledge received
        // ‘0’ = Acknowledge received
        RxACK = 1 << 7,
    };
    AK_ENUM_BITWISE_FRIEND_OPERATORS(StatusRegisterFlags);

    void write_reg(RegisterOffset reg, u8 value)
    {
        auto offset = to_underlying(reg) << m_register_shift;

        // FIXME: What is the endianness for reg-io-width > 1?
        switch (m_register_io_width) {
        case 1:
            *reinterpret_cast<u8 volatile*>(m_register_address.get() + offset) = value;
            break;
        case 2:
            VERIFY(offset % 2 == 0); // FIXME: Check for alignment in try_to_initialize
            *reinterpret_cast<u16 volatile*>(m_register_address.get() + offset) = value;
            break;
        case 4:
            VERIFY(offset % 4 == 0);
            *reinterpret_cast<u32 volatile*>(m_register_address.get() + offset) = value;
            break;
        default:
            VERIFY_NOT_REACHED();
        }

        dbgln("write reg {:#x}, {:#x}", to_underlying(reg), value);
    }

    u8 read_reg(RegisterOffset reg)
    {
        auto offset = to_underlying(reg) << m_register_shift;

        // FIXME: What is the endianness for reg-io-width > 1?
        switch (m_register_io_width) {
        case 1:
            return *reinterpret_cast<u8 volatile*>(m_register_address.get() + offset);
            break;
        case 2:
            VERIFY(offset % 2 == 0);
            return *reinterpret_cast<u16 volatile*>(m_register_address.get() + offset);
            break;
        case 4:
            VERIFY(offset % 4 == 0);
            return *reinterpret_cast<u32 volatile*>(m_register_address.get() + offset);
            break;
        default:
            VERIFY_NOT_REACHED();
        }
    }
};

}
