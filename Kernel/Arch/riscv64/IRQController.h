#pragma once

#include <AK/AtomicRefCounted.h>
#include <AK/StringView.h>
#include <AK/Types.h>

#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel {

class IRQController : public AtomicRefCounted<IRQController> {
public:
    void enable(GenericInterruptHandler const &)
    {
        TODO_RISCV64();
    }

    void disable(GenericInterruptHandler const &)
    {
        TODO_RISCV64();
    }

    void eoi(GenericInterruptHandler const &)
    {
        TODO_RISCV64();
    }

    StringView model() const
    {
        TODO_RISCV64();
    }
};

}
