#include <Kernel/Arch/PowerState.h>
#include <Kernel/Arch/aarch64/RPi/Watchdog.h>

namespace Kernel {

void arch_specific_reboot()
{
    TODO_RISCV64();
}

void arch_specific_poweroff()
{
    TODO_RISCV64();
}

}
