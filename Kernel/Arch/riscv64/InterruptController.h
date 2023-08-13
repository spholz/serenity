/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/StringView.h>
#include <AK/Types.h>
#include <Kernel/Arch/riscv64/IRQController.h>

namespace Kernel::RISCV64 {

class InterruptController : public IRQController {
public:
    InterruptController();

private:
    virtual void enable(GenericInterruptHandler const&) override;
    virtual void disable(GenericInterruptHandler const&) override;

    virtual void eoi(GenericInterruptHandler const&) const override;

    virtual u64 pending_interrupts() const override;

    virtual StringView model() const override
    {
        return "RISC-V Interrupt Controller"sv;
    }
};

}
