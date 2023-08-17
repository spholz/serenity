/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <AK/StringView.h>
#include <AK/Types.h>

namespace Kernel::RISCV64 {

// https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf

// 5.1.11 Supervisor Address Translation and Protection (satp) Register
struct [[gnu::packed]] alignas(u64) Satp {
    enum class Mode : u64 {
        Bare = 0,
        Sv39 = 8,
        Sv48 = 9,
        Sv67 = 10,
    };

    // Physical page number of root page table
    u64 PPN : 44;

    // Address space identifier
    u64 ASID : 16;

    // Current address-translation scheme
    Mode MODE : 4;

    static inline void write(Satp satp)
    {
        asm volatile("csrw satp, %[value]" ::[value] "r"(satp));
    }

    static inline Satp read()
    {
        Satp satp;

        asm volatile("csrr %[value], satp"
                     : [value] "=r"(satp));

        return satp;
    }
};

// 5.1.1 Supervisor Status Register (sstatus)
struct [[gnu::packed]] alignas(u64) Sstatus {
    enum class PrivilegeMode : u64 {
        User = 0,
        Supervisor = 1,
    };

    enum class FloatingPointStatus : u64 {
        Off = 0,
        Initial = 1,
        Clean = 2,
        Dirty = 3,
    };

    enum class VectorStatus : u64 {
        Off = 0,
        Initial = 1,
        Clean = 2,
        Dirty = 3,
    };

    enum class UserModeExtensionsStatus : u64 {
        AllOff = 0,
        NoneDirtyOrClean_SomeOn = 1,
        NoneDirty_SomeOn = 2,
        SomeDirty = 3,
    };

    enum class XLEN : unsigned {
        Bits32 = 1,
        Bits64 = 2,
        Bits128 = 3,
    };

    u64 _reserved0 : 1;

    // Enables or disables all interrupts in supervisor mode
    u64 SIE : 1;

    u64 _reserved2 : 3;

    // Indicates whether supervisor interrupts were enabled prior to trapping into supervisor mode
    // When a trap is taken into supervisor mode, SPIE is set to SIE, and SIE is set to 0. When
    // an SRET instruction is executed, SIE is set to SPIE, then SPIE is set to 1.
    u64 SPIE : 1;

    // Controls the endianness of explicit memory accesses made from
    // U-mode, which may differ from the endianness of memory accesses in S-mode
    u64 UBE : 1;

    u64 _reserved7 : 1;

    // Indicates the privilege level at which a hart was executing before entering supervisor mode
    PrivilegeMode SPP : 1;

    // Encodes the status of the vector extension state, including the vector registers v0–v31 and
    // the CSRs vcsr, vxrm, vxsat, vstart, vl, vtype, and vlenb.
    VectorStatus VS : 2;

    u64 _reserved11 : 2;

    // Encodes the status of the floating-point unit state,
    // including the floating-point registers f0–f31 and the CSRs fcsr, frm, and fflags.
    FloatingPointStatus FS : 2;

    // The XS field encodes the status of additional user-mode extensions and associated state.
    UserModeExtensionsStatus XS : 2;

    u64 _reserved17 : 1;

    // The SUM (permit Supervisor User Memory access) bit modifies the privilege with which S-mode
    // loads and stores access virtual memory. When SUM=0, S-mode memory accesses to pages that are
    // accessible by U-mode (U=1 in Figure 5.18) will fault. When SUM=1, these accesses are permitted.
    // SUM has no effect when page-based virtual memory is not in effect, nor when executing in U-mode.
    // Note that S-mode can never execute instructions from user pages, regardless of the state of SUM.
    u64 SUM : 1;

    // The MXR (Make eXecutable Readable) bit modifies the privilege with which loads access virtual
    // memory. When MXR=0, only loads from pages marked readable (R=1 in Figure 5.18) will succeed.
    // When MXR=1, loads from pages marked either readable or executable (R=1 or X=1) will succeed.
    // MXR has no effect when page-based virtual memory is not in effect.
    u64 MXR : 1;

    u64 _reserved20 : 12;

    // Controls the value of XLEN for U-mode
    XLEN UXL : 2;

    u64 _reserved34 : 29;

    // The SD bit is a read-only bit that summarizes whether either the FS, VS, or XS fields signal the
    // presence of some dirty state that will require saving extended user context to memory.
    u64 SD : 1;

    static inline void write(Sstatus sstatus)
    {
        asm volatile("csrw sstatus, %[value]" ::[value] "r"(sstatus));
    }

    static inline Sstatus read()
    {
        Sstatus sstatus;

        asm volatile("csrr %[value], sstatus"
                     : [value] "=r"(sstatus));

        return sstatus;
    }
};
static_assert(sizeof(Sstatus) == 8);

inline u64 rdtime()
{
    u64 time;

    asm volatile("rdtime %[value]"
                 : [value] "=r"(time));

    return time;
}

static inline StringView scause_to_string(uintptr_t scause)
{
    constexpr u64 INTERRUPT = 1LU << 63;

    switch (scause) {
    case INTERRUPT | 1:
        return "Supervisor software interrupt"sv;
        break;
    case INTERRUPT | 5:
        return "Supervisor timer interrupt"sv;
        break;
    case INTERRUPT | 9:
        return "Supervisor external interrupt"sv;
        break;

    case 0:
        return "Instruction address misaligned"sv;
    case 1:
        return "Instruction access fault"sv;
    case 2:
        return "Illegal instruction"sv;
    case 3:
        return "Breakpoint"sv;
    case 4:
        return "Load address misaligned"sv;
    case 5:
        return "Load access fault"sv;
    case 6:
        return "Store/AMO address misaligned"sv;
    case 7:
        return "Store/AMO access fault"sv;
    case 8:
        return "Environment call from U-mode"sv;
    case 9:
        return "Environment call from S-mode"sv;
    case 12:
        return "Instruction page fault"sv;
    case 13:
        return "Load page fault"sv;
    case 15:
        return "Store/AMO page fault"sv;
    default:
        VERIFY_NOT_REACHED();
    }
}

static inline bool scause_is_page_fault(uintptr_t scause)
{
    return scause == 12 || scause == 13 || scause == 15;
}

}

template<>
struct AK::Formatter<Kernel::RISCV64::Sstatus> : AK::Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, Kernel::RISCV64::Sstatus value)
    {
        if (value.SD)
            TRY(builder.put_literal("SD "sv));

        switch (value.UXL) {
        case Kernel::RISCV64::Sstatus::XLEN::Bits32:
            TRY(builder.put_literal("UXL=32 "sv));
            break;
        case Kernel::RISCV64::Sstatus::XLEN::Bits64:
            TRY(builder.put_literal("UXL=64 "sv));
            break;
        case Kernel::RISCV64::Sstatus::XLEN::Bits128:
            TRY(builder.put_literal("UXL=128 "sv));
            break;
        }

        if (value.MXR)
            TRY(builder.put_literal("MXR "sv));

        if (value.SUM)
            TRY(builder.put_literal("SUM "sv));

        switch (value.XS) {
        case Kernel::RISCV64::Sstatus::UserModeExtensionsStatus::AllOff:
            TRY(builder.put_literal("XS=AllOff "sv));
            break;
        case Kernel::RISCV64::Sstatus::UserModeExtensionsStatus::NoneDirtyOrClean_SomeOn:
            TRY(builder.put_literal("XS=NoneDirtyOrClean_SomeOn "sv));
            break;
        case Kernel::RISCV64::Sstatus::UserModeExtensionsStatus::NoneDirty_SomeOn:
            TRY(builder.put_literal("XS=NoneDirty_SomeOn "sv));
            break;
        case Kernel::RISCV64::Sstatus::UserModeExtensionsStatus::SomeDirty:
            TRY(builder.put_literal("XS=SomeDirty "sv));
            break;
        }

        switch (value.FS) {
        case Kernel::RISCV64::Sstatus::FloatingPointStatus::Off:
            TRY(builder.put_literal("FS=Off "sv));
            break;
        case Kernel::RISCV64::Sstatus::FloatingPointStatus::Initial:
            TRY(builder.put_literal("FS=Initial "sv));
            break;
        case Kernel::RISCV64::Sstatus::FloatingPointStatus::Clean:
            TRY(builder.put_literal("FS=Clean "sv));
            break;
        case Kernel::RISCV64::Sstatus::FloatingPointStatus::Dirty:
            TRY(builder.put_literal("FS=Dirty "sv));
            break;
        }

        switch (value.VS) {
        case Kernel::RISCV64::Sstatus::VectorStatus::Off:
            TRY(builder.put_literal("VS=Off "sv));
            break;
        case Kernel::RISCV64::Sstatus::VectorStatus::Initial:
            TRY(builder.put_literal("VS=Initial "sv));
            break;
        case Kernel::RISCV64::Sstatus::VectorStatus::Clean:
            TRY(builder.put_literal("VS=Clean "sv));
            break;
        case Kernel::RISCV64::Sstatus::VectorStatus::Dirty:
            TRY(builder.put_literal("VS=Dirty "sv));
            break;
        }

        switch (value.SPP) {
        case Kernel::RISCV64::Sstatus::PrivilegeMode::User:
            TRY(builder.put_literal("SPP=User "sv));
            break;
        case Kernel::RISCV64::Sstatus::PrivilegeMode::Supervisor:
            TRY(builder.put_literal("SPP=Supervisor "sv));
            break;
        }

        if (value.UBE)
            TRY(builder.put_literal("UBE "sv));

        if (value.SPIE)
            TRY(builder.put_literal("SPIE "sv));

        if (value.SIE)
            TRY(builder.put_literal("SIE "sv));

        TRY(builder.put_literal("("sv));
        TRY(builder.put_u64(*bit_cast<u64*>(&value), 16, true));
        TRY(builder.put_literal(")"sv));

        return {};
    }
};
