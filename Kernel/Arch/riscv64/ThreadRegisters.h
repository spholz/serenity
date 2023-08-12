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
    u64 sepc;
    u64 sp;
    u64 satp;

    FlatPtr ip() const { return sepc; }
    void set_ip(FlatPtr value) { sepc = value; }
    void set_sp(FlatPtr value) { sp = value; }

    void set_initial_state(bool is_kernel_process, Memory::AddressSpace& space, FlatPtr kernel_stack_top)
    {
        set_sp(kernel_stack_top);
        satp = space.page_directory().satp();
        set_sstatus(is_kernel_process);
    }

    void set_entry_function(FlatPtr entry_ip, FlatPtr entry_data)
    {
        set_ip(entry_ip);
        x[10] = entry_data; // a0
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
        RiscV64::Sstatus saved_sstatus = {};

        // Don't mask any interrupts, so all interrupts are enabled when transfering into the new context
        saved_sstatus.SIE = 1;

        saved_sstatus.SPP = is_kernel_process ? RiscV64::Sstatus::PrivilegeMode::Supervisor : RiscV64::Sstatus::PrivilegeMode::User;
        memcpy(&sstatus, &saved_sstatus, sizeof(u64));
    }
};

}
