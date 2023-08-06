/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com, SbiError>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>

// RISC-V Supervisor Binary Interface Specification

namespace Kernel::SBI {

// Chapter 3. Binary Encoding
enum class SbiError {
    Success = 0,           // SBI_SUCCESS:               Completed successfully
    Failed = -1,           // SBI_ERR_FAILED:            Failed
    NotSupported = -2,     // SBI_ERR_NOT_SUPPORTED:     Not supported
    InvalidParam = -3,     // SBI_ERR_INVALID_PARAM:     Invalid parameter(s)
    Denied = -4,           // SBI_ERR_DENIED:            Denied or not allowed
    InvalidAddress = -5,   // SBI_ERR_INVALID_ADDRESS:   Invalid address(s)
    AlreadyAvailable = -6, // SBI_ERR_ALREADY_AVAILABLE: Already available
    AlreadyStarted = -7,   // SBI_ERR_ALREADY_STARTED:   Already started
    AlreadyStopped = -8,   // SBI_ERR_ALREADY_STOPPED:   Already stopped
    NoShmem = -9,          // SBI_ERR_NO_SHMEM:          Shared memory not available
};

enum class EId {
    Base = 0x10,               // Base Extension (EID #0x10)
    DebugConsole = 0x4442434E, // Debug Console Extension (EID #0x4442434E "DBCN")
    Timer = 0x54494D45,        // Timer Extension (EID #0x54494D45 "TIME")
};

ErrorOr<long, SbiError> sbi_ecall0(EId extension_id, u32 function_id);
ErrorOr<long, SbiError> sbi_ecall1(EId extension_id, u32 function_id, long arg0);

// Chapter 4. Base Extension (EID #0x10)
// Required extension since SBI v0.2
namespace Base {

enum class FId {
    GetSpecVersion = 0,
    GetImplId = 1,
    GetImplVersion = 2,
    ProbeExtension = 3,
    GetMvendorid = 4,
    GetMarchid = 5,
    GetMimpid = 6,
};

struct [[gnu::packed]] SbiSpecificationVersion {
    long minor : 24;
    long major : 7;
    long reserved : 1;
};

// Get SBI specification version (FID #0)
// Returns the current SBI specification version. This function must always succeed. The minor
// number of the SBI specification is encoded in the low 24 bits, with the major number encoded in
// the next 7 bits. Bit 31 must be 0 and is reserved for future expansion.
ErrorOr<SbiSpecificationVersion, SbiError> get_spec_version();

// Get SBI implementation ID (FID #1)
// Returns the current SBI implementation ID, which is different for every SBI implementation. It is
// intended that this implementation ID allows software to probe for SBI implementation quirks.
ErrorOr<long, SbiError> get_impl_id();

// Get SBI implementation version (FID #2)
// Returns the current SBI implementation version. The encoding of this version number is specific to
// the SBI implementation.
ErrorOr<long, SbiError> get_impl_version();

// Probe SBI extension (FID #3)
// Returns 0 if the given SBI extension ID (EID) is not available, or 1 if it is available unless defined as
// any other non-zero value by the implementation.
ErrorOr<long, SbiError> probe_extension(long extension_id);

// Get machine vendor ID (FID #4)
// Return a value that is legal for the mvendorid CSR and 0 is always a legal value for this CSR.
ErrorOr<long, SbiError> get_mvendorid();

// Get machine architecture ID (FID #5)
// Return a value that is legal for the marchid CSR and 0 is always a legal value for this CSR.
ErrorOr<long, SbiError> get_marchid();

// Get machine implementation ID (FID #6)
// Return a value that is legal for the mimpid CSR and 0 is always a legal value for this CSR.
ErrorOr<long, SbiError> get_mimpid();

}

// Chapter 5. Legacy Extensions (EIDs #0x00 - #0x0F)
namespace Legacy {

enum class LegacyEId {
    SetTimer = 0,
    ConsolePutchar = 1,
    ConsoleGetchar = 2,
    ClearIpi = 3,
    SendIpi = 4,
    RemoteFencei = 5,
    RemoteSfencevma = 6,
    RemoteSfancevmaWithAsid = 7,
    SystemShutdown = 8,
};

// Write data present in ch to debug console.
ErrorOr<void, long> console_putchar(int ch);

}

// Chapter 6. Timer Extension (EID #0x54494D45 "TIME")
// Since SBI v0.2
namespace Timer {

enum class FId {
    SetTimer = 0,
};

// Set Timer (FID #0)
// Programs the clock for next event after stime_value time. stime_value is in absolute time. This
// function must clear the pending timer interrupt bit as well.
ErrorOr<void, SbiError> set_timer(u64 stime_value);

}

// Chapter 12. Debug Console Extension (EID #0x4442434E "DBCN")
// Since SBI v2.0
namespace DBCN {

enum class FId {
    DebugConsoleWrite = 0,
    DebugConsoleRead = 1,
    DebugConsoleWriteByte = 2,
};

// Console Write (FID #0)
// Write bytes to the debug console from input memory.
ErrorOr<long, SbiError> debug_console_write(unsigned long num_bytes, unsigned long base_addr_lo, unsigned long base_addr_hi);

// Console Read (FID #1)
// Read bytes from the debug console into an output memory.
ErrorOr<long, SbiError> debug_console_read(unsigned long num_bytes, unsigned long base_addr_lo, unsigned long base_addr_hi);

// Console Write Byte (FID #2)
ErrorOr<void, SbiError> debug_console_write_byte(u8 byte);

}

void initialize();

}
