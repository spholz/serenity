/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com, SbiError>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/riscv64/SBI.h>

namespace Kernel::SBI {

ErrorOr<long, SbiError> sbi_ecall0(EId extension_id, u32 function_id)
{
    register uintptr_t a0 asm("a0");
    register uintptr_t a1 asm("a1");
    register uintptr_t a6 asm("a6") = function_id;
    register uintptr_t a7 asm("a7") = to_underlying(extension_id);
    asm volatile("ecall"
                 : "=r"(a0), "=r"(a1)
                 : "r"(a6), "r"(a7)
                 : "memory");
    if (a0 == to_underlying(SbiError::Success)) {
        long result = (long)a1;
        return result;
    }

    return (SbiError)a1;
}

// TODO: this jumps to an absolute address in the error case if EXTRA_KERNEL_DEBUG_SYMBOLS is on???
ErrorOr<long, SbiError> sbi_ecall1(EId extension_id, u32 function_id, long arg0)
{
    register uintptr_t a0 asm("a0") = arg0;
    register uintptr_t a1 asm("a1");
    register uintptr_t a6 asm("a6") = function_id;
    register uintptr_t a7 asm("a7") = to_underlying(extension_id);
    asm volatile("ecall"
                 : "=r"(a0), "=r"(a1)
                 : "r"(a0), "r"(a6), "r"(a7)
                 : "memory");
    if (a0 == to_underlying(SbiError::Success)) {
        long result = (long)a1;
        return result;
    }

    return (SbiError)a1;
}

namespace Legacy {

ErrorOr<void, long> console_putchar(int ch)
{
    register uintptr_t a0 asm("a0") = ch;
    register uintptr_t a7 asm("a7") = to_underlying(LegacyEId::ConsolePutchar);
    asm volatile("ecall"
                 : "=r"(a0)
                 : "r"(a7)
                 : "memory");
    if (a0 == 0)
        return {};

    return (long)a0;
}

}

namespace Timer {

ErrorOr<void, SbiError> set_timer(u64 stime_value)
{
    const auto result = SBI::sbi_ecall1(EId::Timer, to_underlying(FId::SetTimer), stime_value);
    if (result.is_error()) {
        return result.error();
    }

    return {};
}

}

namespace DBCN {

ErrorOr<void, SbiError> debug_console_write_byte(u8 byte)
{
    const auto result = SBI::sbi_ecall1(EId::DebugConsole, to_underlying(FId::DebugConsoleWriteByte), byte);
    if (result.is_error()) {
        return result.error();
    }

    return {};
}

}
}
