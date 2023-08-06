#pragma once

#include <Kernel/Arch/riscv64/Registers.h>

namespace Kernel::RiscV64::Asm {

inline void set_satp(FlatPtr satp)
{
    asm volatile("csrw satp, %[value]" ::[value] "r"(satp));
}

}
