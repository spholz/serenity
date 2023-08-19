#include <AK/Singleton.h>

#include <Kernel/Arch/Delay.h>
#include <Kernel/Bus/PCI/Initializer.h>
#include <Kernel/Sections.h>
#include <Kernel/Tasks/Process.h>
#include <Kernel/kstdio.h>

// Delay.cpp
namespace Kernel {

void microseconds_delay(u32)
{
    TODO_AARCH64();
}

}

// Initializer.cpp
namespace Kernel::PCI {

bool g_pci_access_io_probe_failed { false };
bool g_pci_access_is_disabled_from_commandline { true };

void initialize()
{
    TODO_RISCV64();
}

}

// kprintf.cpp
void set_serial_debug_enabled(bool)
{
    TODO_RISCV64();
}

extern "C" int ffs(int);
extern "C" int ffs(int)
{
    TODO_RISCV64();
}

namespace Kernel::ACPI::StaticParsing {
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations();
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations()
{
    TODO_RISCV64();
}
}

extern "C" short unsigned int __atomic_fetch_sub_2(volatile void *ptr, short unsigned int val, int memorder)
{
    (void)ptr;
    (void)val;
    (void)memorder;
    TODO_RISCV64();
}

extern "C" short unsigned int __atomic_fetch_add_2(volatile void *ptr, short unsigned int val, int memorder)
{
    (void)ptr;
    (void)val;
    (void)memorder;
    TODO_RISCV64();
}

extern "C" unsigned char __atomic_exchange_1(volatile void *ptr, unsigned char val, int memorder)
{
    (void)memorder;
    unsigned char const previous_value = *((volatile unsigned char *)ptr);
    *((volatile unsigned char *)ptr) = val;
    return previous_value;
}

extern "C" bool __atomic_compare_exchange_1(volatile void *ptr, void *expected, unsigned char desired, bool weak, int success_memorder, int failure_memorder)
{
    (void)ptr;
    (void)expected;
    (void)desired;
    (void)weak;
    (void)success_memorder;
    (void)failure_memorder;
    TODO_RISCV64();
}

