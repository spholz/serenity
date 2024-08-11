/*
 * Copyright (c) 2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Format.h>
#include <Kernel/Arch/PowerState.h>
#include <Kernel/Arch/Processor.h>
#include <Kernel/Boot/CommandLine.h>
#include <Kernel/KSyms.h>
#include <Kernel/Library/Panic.h>
#include <Kernel/Tasks/Thread.h>

namespace Kernel {

void __panic(char const* file, unsigned int line, char const* function)
{
    // Avoid lock ranking checks on crashing paths, just try to get some debugging messages out.
    auto* thread = Thread::current();
    if (thread)
        thread->set_crashing();

    critical_dmesgln("at {}:{} in {}", file, line, function);
    dump_backtrace(PrintToScreen::Yes);
    if (!CommandLine::was_initialized())
        Processor::halt();
    switch (kernel_command_line().panic_mode()) {
    case PanicMode::Shutdown:
        arch_specific_poweroff(PowerOffOrRebootReason::SystemFailure);

        // Note: If we failed to invoke platform shutdown, we need to halt afterwards
        // to ensure no further execution on any CPU still happens.
        [[fallthrough]];
    case PanicMode::Halt:
    default:
#if ARCH(X86_64)
        auto send_i8042 = [](u8 port, u8 byte) {
            while ((IO::in8(0x64) & 0b10) != 0)
                Processor::wait_check();

            IO::out8(port, byte);
        };

        send_i8042(0x60, 0xed);
        send_i8042(0x60, 0b111);
#endif
        Processor::halt();
    }
}
}
