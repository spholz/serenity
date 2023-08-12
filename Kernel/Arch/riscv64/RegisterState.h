/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sys/arch/riscv64/regs.h>

#include <Kernel/Arch/riscv64/Registers.h>
#include <Kernel/Security/ExecutionMode.h>

#include <AK/Platform.h>
VALIDATE_IS_RISCV64()

namespace Kernel {

struct RegisterState {
    u64 x[31]; // Saved general purpose registers
    RiscV64::Sstatus sstatus;
    u64 sepc;

    FlatPtr userspace_sp() const
    {
        TODO_RISCV64();
    }

    void set_userspace_sp([[maybe_unused]] FlatPtr value)
    {
        TODO_RISCV64();
    }

    FlatPtr ip() const { return sepc; }
    void set_ip(FlatPtr value) { sepc = value; }
    FlatPtr bp() const { TODO_RISCV64(); }

    ExecutionMode previous_mode() const
    {
        switch (sstatus.SPP) {
        case RiscV64::Sstatus::PrivilegeMode::User:
            return ExecutionMode::User;
        case RiscV64::Sstatus::PrivilegeMode::Supervisor:
            return ExecutionMode::Kernel;
        }
    }

    void set_return_reg([[maybe_unused]] FlatPtr value) { TODO_RISCV64(); }
    void capture_syscall_params([[maybe_unused]] FlatPtr& function, [[maybe_unused]] FlatPtr& arg1, [[maybe_unused]] FlatPtr& arg2, [[maybe_unused]] FlatPtr& arg3, [[maybe_unused]] FlatPtr& arg4) const
    {
        TODO_RISCV64();
    }
};

#define REGISTER_STATE_SIZE (33 * 8)
static_assert(AssertSize<RegisterState, REGISTER_STATE_SIZE>());

inline void copy_kernel_registers_into_ptrace_registers([[maybe_unused]] PtraceRegisters& ptrace_regs, [[maybe_unused]] RegisterState const& kernel_regs)
{
    TODO_RISCV64();
}

inline void copy_ptrace_registers_into_kernel_registers([[maybe_unused]] RegisterState& kernel_regs, [[maybe_unused]] PtraceRegisters const& ptrace_regs)
{
    TODO_RISCV64();
}

struct DebugRegisterState {
};

}
