/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

#include <Kernel/Arch/PCIMSI.h>

namespace Kernel {

u64 msi_address_register([[maybe_unused]] u8 destination_id, [[maybe_unused]] bool redirection_hint, [[maybe_unused]] bool destination_mode)
{
    TODO_RISCV64();
}

u32 msi_data_register([[maybe_unused]] u8 vector, [[maybe_unused]] bool level_trigger, [[maybe_unused]] bool assert)
{
    TODO_RISCV64();
}

u32 msix_vector_control_register([[maybe_unused]] u32 vector_control, [[maybe_unused]] bool mask)
{
    TODO_RISCV64();
}

void msi_signal_eoi()
{
    TODO_RISCV64();
}

}
