#include <AK/Types.h>
#include <Kernel/Memory/AddressSpace.h>

namespace Kernel {

struct ThreadRegisters {
    u64 x[31];

    FlatPtr ip() const { TODO_RISCV64(); }
    void set_ip([[maybe_unused]] FlatPtr value) { TODO_RISCV64(); }
    void set_sp([[maybe_unused]] FlatPtr value) { TODO_RISCV64(); }

    void set_initial_state([[maybe_unused]] bool is_kernel_process, [[maybe_unused]] Memory::AddressSpace& space, [[maybe_unused]] FlatPtr kernel_stack_top)
    {
        TODO_RISCV64();
    }

    void set_entry_function([[maybe_unused]] FlatPtr entry_ip, [[maybe_unused]] FlatPtr entry_data)
    {
        TODO_RISCV64();
    }

    void set_exec_state([[maybe_unused]] FlatPtr entry_ip, [[maybe_unused]] FlatPtr userspace_sp, [[maybe_unused]] Memory::AddressSpace& space)
    {
        TODO_RISCV64();
    }
};

}
