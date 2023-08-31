/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Types.h>
#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Library/StdLib.h>
#include <Kernel/Memory/AddressSpace.h>

namespace Kernel {

struct ThreadRegisters {
    u64 x[31];
    u64 sstatus;
    u64 pc;
    u64 satp;

    u64 kernel_sp;

    FlatPtr ip() const { return pc; }
    void set_ip(FlatPtr value) { pc = value; }

    FlatPtr sp() const { return x[1]; }
    void set_sp(FlatPtr value) { x[1] = value; }

    void set_initial_state(bool is_kernel_process, Memory::AddressSpace& space, FlatPtr kernel_stack_top)
    {
        set_sp(kernel_stack_top);
        satp = space.page_directory().satp();
        set_sstatus(is_kernel_process);
    }

    void set_entry_function(FlatPtr entry_ip, FlatPtr entry_data)
    {
        set_ip(entry_ip);
        x[9] = entry_data; // a0
    }

    void set_exec_state(FlatPtr entry_ip, FlatPtr userspace_sp, Memory::AddressSpace& space)
    {
        set_ip(entry_ip);
        set_sp(userspace_sp);
        satp = space.page_directory().satp();
        set_sstatus(false);
    }

    void set_sstatus(bool is_kernel_process)
    {
        RISCV64::Sstatus sstatus = {};

        // Enable interrupts
        sstatus.SPIE = 1;

        sstatus.FS = RISCV64::Sstatus::FloatingPointStatus::Initial;

        sstatus.SPP = is_kernel_process ? RISCV64::Sstatus::PrivilegeMode::Supervisor : RISCV64::Sstatus::PrivilegeMode::User;
        sstatus.UXL = RISCV64::Sstatus::XLEN::Bits64;

        memcpy(&this->sstatus, &sstatus, sizeof(RISCV64::Sstatus));
    }
};

}
