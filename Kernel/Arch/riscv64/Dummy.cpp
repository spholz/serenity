#include <AK/Singleton.h>

#include <Kernel/Arch/DebugOutput.h>
#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/riscv64/SBI.h>
#include <Kernel/Bus/PCI/Initializer.h>
#include <Kernel/Sections.h>
#include <Kernel/Tasks/Process.h>
#include <Kernel/kstdio.h>

namespace Kernel {

void microseconds_delay(u32 microseconds)
{
    (void)microseconds;
    // for (u32 i = 0; i < microseconds; i++)
    //     asm volatile("");
}

void debug_output(char ch)
{
    (void)SBI::Legacy::console_putchar(ch);
}

}

// void set_serial_debug_enabled(bool)
// {
// }

namespace Kernel::ACPI::StaticParsing {
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations();
ErrorOr<Optional<PhysicalAddress>> find_rsdp_in_platform_specific_memory_locations()
{
    // FIXME: Implement finding RSDP for riscv64.
    return Optional<PhysicalAddress> {};
}
}

extern "C" u16 __atomic_fetch_sub_2(void volatile* ptr, u16 val, int memorder)
{
    (void)memorder;
    u16 previous_value = *(reinterpret_cast<u16 volatile*>(ptr));
    *(reinterpret_cast<u16 volatile*>(ptr)) -= val;
    return previous_value;
}

extern "C" u16 __atomic_fetch_add_2(void volatile* ptr, u16 val, int memorder)
{
    (void)memorder;
    u16 previous_value = *(reinterpret_cast<u16 volatile*>(ptr));
    *(reinterpret_cast<u16 volatile*>(ptr)) += val;
    return previous_value;
}

extern "C" u8 __atomic_exchange_1(void volatile* ptr, u8 val, int memorder)
{
    (void)memorder;
    u8 previous_value = *(reinterpret_cast<u8 volatile*>(ptr));
    *(reinterpret_cast<u8 volatile*>(ptr)) = val;
    return previous_value;
}

extern "C" bool __atomic_compare_exchange_1(void volatile* ptr, void* expected, u8 desired, bool weak, int success_memorder, int failure_memorder)
{
    (void)ptr;
    (void)expected;
    (void)desired;
    (void)weak;
    (void)success_memorder;
    (void)failure_memorder;
    TODO_RISCV64();
}
