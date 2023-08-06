#pragma once

#include <AK/Format.h>
#include <AK/StringView.h>
#include <AK/Types.h>

namespace Kernel::RiscV64 {
struct [[gnu::packed]] alignas(u64) Sstatus {
    enum class PrivilegeMode : u8 {
        User = 0,
        Supervisor = 1,
        // Machine = 3,
    };

    enum class FloatingPointStatus : u8 {
        Off = 0,
        Initial = 1,
        Clean = 2,
        Dirty = 3,
    };

    enum class VectorStatus : u8 {
        Off = 0,
        Initial = 1,
        Clean = 2,
        Dirty = 3,
    };

    enum class UserModeExtensionsStatus : u8 {
        AllOff = 0,
        NoneDirtyOrClean_SomeOn = 1,
        NoneDirty_SomeOn = 2,
        SomeDirty = 3,
    };

    enum class XLEN : u8 {
        _32 = 1,
        _64 = 2,
        _128 = 3,
    };

    u64 _reserved0 : 1;
    u64 SIE : 1;
    u64 _reserved2 : 3;
    u64 SPIE : 1;
    u64 UBE : 1;
    u64 _reserved7 : 1;
    PrivilegeMode SPP : 1;
    VectorStatus VS : 2;
    u64 _reserved11 : 2;
    FloatingPointStatus FS : 2;
    UserModeExtensionsStatus XS : 2;
    u64 _reserved17 : 1;
    u64 SUM : 1;
    u64 MXR : 1;
    u64 _reserved20 : 12;
    XLEN UXL : 2;
    u64 _reserved34 : 29;
    u64 SD : 1;

    static inline void write(Sstatus spsr_el3)
    {
        asm volatile("csrw sstatus, %[value]" ::[value] "r"(spsr_el3));
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
    constexpr uintptr_t INTERRUPT = 1LU << 63;

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


}

template<>
struct AK::Formatter<Kernel::RiscV64::Sstatus> : AK::Formatter<FormatString> {
    ErrorOr<void> format(FormatBuilder& builder, Kernel::RiscV64::Sstatus value)
    {
        if (value.SD)
            TRY(builder.put_literal("SD "sv));

        switch (value.UXL) {
        case Kernel::RiscV64::Sstatus::XLEN::_32:
            TRY(builder.put_literal("UXL=32 "sv));
            break;
        case Kernel::RiscV64::Sstatus::XLEN::_64:
            TRY(builder.put_literal("UXL=64 "sv));
            break;
        case Kernel::RiscV64::Sstatus::XLEN::_128:
            TRY(builder.put_literal("UXL=128 "sv));
            break;
        }

        if (value.MXR)
            TRY(builder.put_literal("MXR "sv));

        if (value.SUM)
            TRY(builder.put_literal("SUM "sv));

        switch (value.XS) {
        case Kernel::RiscV64::Sstatus::UserModeExtensionsStatus::AllOff:
            TRY(builder.put_literal("XS=AllOff "sv));
            break;
        case Kernel::RiscV64::Sstatus::UserModeExtensionsStatus::NoneDirtyOrClean_SomeOn:
            TRY(builder.put_literal("XS=NoneDirtyOrClean_SomeOn "sv));
            break;
        case Kernel::RiscV64::Sstatus::UserModeExtensionsStatus::NoneDirty_SomeOn:
            TRY(builder.put_literal("XS=NoneDirty_SomeOn "sv));
            break;
        case Kernel::RiscV64::Sstatus::UserModeExtensionsStatus::SomeDirty:
            TRY(builder.put_literal("XS=SomeDirty "sv));
            break;
        }

        switch (value.FS) {
        case Kernel::RiscV64::Sstatus::FloatingPointStatus::Off:
            TRY(builder.put_literal("FS=Off "sv));
            break;
        case Kernel::RiscV64::Sstatus::FloatingPointStatus::Initial:
            TRY(builder.put_literal("FS=Initial "sv));
            break;
        case Kernel::RiscV64::Sstatus::FloatingPointStatus::Clean:
            TRY(builder.put_literal("FS=Clean "sv));
            break;
        case Kernel::RiscV64::Sstatus::FloatingPointStatus::Dirty:
            TRY(builder.put_literal("FS=Dirty "sv));
            break;
        }

        switch (value.VS) {
        case Kernel::RiscV64::Sstatus::VectorStatus::Off:
            TRY(builder.put_literal("VS=Off "sv));
            break;
        case Kernel::RiscV64::Sstatus::VectorStatus::Initial:
            TRY(builder.put_literal("VS=Initial "sv));
            break;
        case Kernel::RiscV64::Sstatus::VectorStatus::Clean:
            TRY(builder.put_literal("VS=Clean "sv));
            break;
        case Kernel::RiscV64::Sstatus::VectorStatus::Dirty:
            TRY(builder.put_literal("VS=Dirty "sv));
            break;
        }

        switch (value.SPP) {
        case Kernel::RiscV64::Sstatus::PrivilegeMode::User:
            TRY(builder.put_literal("SPP=User "sv));
            break;
        case Kernel::RiscV64::Sstatus::PrivilegeMode::Supervisor:
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
