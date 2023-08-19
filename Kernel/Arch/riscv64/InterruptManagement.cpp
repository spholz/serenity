#include <Kernel/Arch/riscv64/IRQController.h>
#include <Kernel/Arch/riscv64/InterruptManagement.h>

namespace Kernel {

bool InterruptManagement::initialized()
{
    TODO_RISCV64();
}

InterruptManagement& InterruptManagement::the()
{
    TODO_RISCV64();
}

void InterruptManagement::initialize()
{
    TODO_RISCV64();
}

u8 InterruptManagement::acquire_mapped_interrupt_number(u8)
{
    TODO_RISCV64();
}

Vector<NonnullLockRefPtr<IRQController>> const& InterruptManagement::controllers()
{
    TODO_RISCV64();
}

NonnullLockRefPtr<IRQController> InterruptManagement::get_responsible_irq_controller(u8)
{
    TODO_RISCV64();
}

void InterruptManagement::enumerate_interrupt_handlers(Function<void(GenericInterruptHandler&)>)
{
    TODO_RISCV64();
}

}
