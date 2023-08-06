#pragma once

#include <AK/AtomicRefCounted.h>
#include <AK/StringView.h>
#include <AK/Types.h>

#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel {

class IRQController : public AtomicRefCounted<IRQController> {
public:
    virtual ~IRQController() = default;

    virtual void enable(GenericInterruptHandler const&) = 0;
    virtual void disable(GenericInterruptHandler const&) = 0;

    virtual void eoi(GenericInterruptHandler const&) const = 0;

    virtual u64 pending_interrupts() const = 0;

    virtual StringView model() const = 0;

protected:
    IRQController() = default;
};

}
