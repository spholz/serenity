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
    u64 x[31];
    u64 sstatus;
    u64 pc;

    u64 user_sp;

    FlatPtr userspace_sp() const { return user_sp; }
    void set_userspace_sp(FlatPtr value) { user_sp = value; }

    FlatPtr ip() const { return pc; }
    void set_ip(FlatPtr value) { pc = value; }

    FlatPtr bp() const { return x[7]; }
    void set_bp(FlatPtr value) { x[7] = value; }

    ExecutionMode previous_mode() const
    {
        switch (bit_cast<RISCV64::Sstatus>(sstatus).SPP) {
        case RISCV64::Sstatus::PrivilegeMode::User:
            return ExecutionMode::User;
        case RISCV64::Sstatus::PrivilegeMode::Supervisor:
            return ExecutionMode::Kernel;
        default:
            VERIFY_NOT_REACHED();
        }
    }

    void set_return_reg(FlatPtr value) { x[9] = value; }
    void capture_syscall_params(FlatPtr& function, FlatPtr& arg1, FlatPtr& arg2, FlatPtr& arg3, FlatPtr& arg4) const
    {
        function = x[16];
        arg1 = x[9];
        arg2 = x[10];
        arg3 = x[11];
        arg4 = x[12];
    }
};

#define REGISTER_STATE_SIZE (34 * 8)
static_assert(AssertSize<RegisterState, REGISTER_STATE_SIZE>());

inline void copy_kernel_registers_into_ptrace_registers([[maybe_unused]] PtraceRegisters& ptrace_regs, [[maybe_unused]] RegisterState const& kernel_regs)
{
    for (auto i = 0; i < 31; i++)
        ptrace_regs.x[i] = kernel_regs.x[i];

    ptrace_regs.sp = kernel_regs.userspace_sp();
    ptrace_regs.pc = kernel_regs.ip();
}

inline void copy_ptrace_registers_into_kernel_registers([[maybe_unused]] RegisterState& kernel_regs, [[maybe_unused]] PtraceRegisters const& ptrace_regs)
{
    for (auto i = 0; i < 31; i++)
        kernel_regs.x[i] = ptrace_regs.x[i];

    kernel_regs.set_userspace_sp(ptrace_regs.sp);
    kernel_regs.set_ip(ptrace_regs.pc);
}

struct DebugRegisterState {
};

}
