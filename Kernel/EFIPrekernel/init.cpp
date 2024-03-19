/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/CharacterTypes.h>
#include <AK/UnicodeUtils.h>
#include <Kernel/EFIPrekernel/EFI/EFI.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/LoadedImage.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/MediaAccess.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/RISCVBootProtocol.h>
#include <Kernel/EFIPrekernel/EFI/SystemTable.h>
#include <LibELF/ELFABI.h>
#include <LibELF/Relocation.h>

// TODO: Print load address of kernel and maybe prekernel?

// FIXME: Merge the EFI Prekernel with the x86 Prekernel once the EFI Prekernel works on x86.
//        Making this Prekernel work on x86 requires refactoring the x86 boot info to not rely on multiboot.
//        And for AArch64 we need to make the Kernel bootable from any load address.

// FIXME: We should introduce another Kernel entry point for AArch64 and RISC-V, so we can pass UEFI-related info to the kernel.
//        This is required to be able to use UEFI runtime services and the EFI_GRAPHICS_OUTPUT_PROTOCOL.

// FIXME: Initialize the __stack_chk_guard with a random value via the EFI_RNG_PROTOCOL or other arch-specific methods.
uintptr_t __stack_chk_guard __attribute__((used)) = 0xc6c7c8c9;

void operator delete(void*) { }
void operator delete(void*, unsigned long) { }

extern "C" int __cxa_atexit(void (*)(void*), void*, void*);
extern "C" int __cxa_atexit(void (*)(void*), void*, void*)
{
    return 0;
}

namespace Prekernel {

static EFI::Handle s_image_handle;
static EFI::SystemTable* s_system_table;

static void efi_dbgln(char16_t const* message)
{
    if (s_system_table == nullptr || s_system_table->con_out == nullptr)
        return;

    s_system_table->con_out->output_string(s_system_table->con_out, const_cast<char16_t*>(message));
    s_system_table->con_out->output_string(s_system_table->con_out, const_cast<char16_t*>(u"\r\n"));
}

[[noreturn]] static void halt()
{
    for (;;) {
#if ARCH(AARCH64)
        asm volatile("msr daifset, #2; wfi");
#elif ARCH(RISCV64)
        asm volatile("csrw sie, zero; wfi");
#elif ARCH(X86_64)
        asm volatile("cli; hlt");
#else
#    error Unknown architecture
#endif
    }
}

extern "C" [[noreturn]] void __stack_chk_fail();
extern "C" [[noreturn]] void __stack_chk_fail()
{
    halt();
}

static ErrorOr<void*, EFI::Status> load_kernel(EFI::Handle device_handle)
{
    auto simple_file_system_protocol_guid = EFI::SimpleFileSystemProtocol::guid;
    EFI::SimpleFileSystemProtocol* simple_file_system_interface;
    if (auto status = s_system_table->boot_services->handle_protocol(device_handle, &simple_file_system_protocol_guid, reinterpret_cast<void**>(&simple_file_system_interface)); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get simple filesystem protocol for boot volume");
        return status;
    }

    EFI::FileProtocol* volume;
    if (auto status = simple_file_system_interface->open_volume(simple_file_system_interface, &volume); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to open boot volume");
        return status;
    }

    // FIXME: Get the kernel file name from the command line.
#if ARCH(RISCV64)
    char16_t const kernel_file_name[] = u"boot\\Kernel.bin";
#else
    char16_t const kernel_file_name[] = u"boot\\Kernel";
#endif

    EFI::FileProtocol* file;
    if (auto status = volume->open(volume, &file, const_cast<char16_t*>(kernel_file_name), EFI::FileOpenMode::Read, EFI::FileAttribute::None); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to open the kernel image file");
        return status;
    }

    auto file_info_guid = EFI::FileInfo::guid;
    alignas(EFI::FileInfo) u8 info_buffer[sizeof(EFI::FileInfo) + sizeof(kernel_file_name)];
    FlatPtr info_size = sizeof(info_buffer);
    if (auto status = file->get_info(file, &file_info_guid, &info_size, &info_buffer); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get info for the kernel image file");
        return status;
    }

    auto const* info = reinterpret_cast<EFI::FileInfo const*>(info_buffer);
    FlatPtr kernel_size = info->file_size;

#if ARCH(AARCH64) || ARCH(RISCV64)
    // The AArch64 and RISC-V kernel use some memory after the kernel image for the stack and initial page tables.
    // FIXME: Don't hardcode additional padding after the kernel.
    //        Either directly jump to the kernel init() like on x86 (and therefore don't use pre_init)
    //        or add some kind of header to the kernel image?
    kernel_size += 12 * MiB;
#endif

    FlatPtr const kernel_page_count = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;

    EFI::PhysicalAddress kernel_image_address = 0;
    if (auto status = s_system_table->boot_services->allocate_pages(EFI::AllocateType::AnyPages, EFI::MemoryType::LoaderData, kernel_page_count, &kernel_image_address); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to allocate pages for the kernel image");
        return status;
    }

    auto* kernel_image = bit_cast<void*>(kernel_image_address);

    // FIXME: Load the kernel in chunks. Loading the entire kernel at once is quite slow on edk2 running on x86.
    efi_dbgln(u"Loading the kernel image...");
    if (auto status = file->read(file, &kernel_size, kernel_image); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to read kernel image file");
        return status;
    }
    efi_dbgln(u"Done");

    return kernel_image;
}

struct Base {
    virtual ~Base() = default;
    virtual void foo() {};
};

struct Derived : Base {
    virtual ~Derived() = default;
    void foo() override
    {
        efi_dbgln(u"test");
    }
};

void test(Base*);
void test(Base* a)
{
    a->foo();
}

Base* test2();
Base* test2()
{
    static Derived b;
    return &b;
}

static u8 bss_test[4096];

extern "C" EFIAPI EFI::Status init(EFI::Handle image_handle, EFI::SystemTable* system_table);
extern "C" EFIAPI EFI::Status init(EFI::Handle image_handle, EFI::SystemTable* system_table)
{
    // FIXME: Clean up memory/resources before returning errors.

    // FIXME: What minimum version do we require?
    static constexpr u32 efi_version_1_10 = (1 << 16) | 10;
    if (system_table->hdr.signature != EFI::SystemTable::signature || system_table->hdr.revision < efi_version_1_10)
        return EFI::Status::Unsupported;

    s_image_handle = image_handle;
    s_system_table = system_table;

    auto* boot_services = system_table->boot_services;

    // clang-format off
    system_table->con_out->set_attribute(system_table->con_out, EFI::TextAttribute {
        .foreground_color = EFI::TextAttribute::ForegroundColor::White,
        .background_color = EFI::TextAttribute::BackgroundColor::Black,
    });
    // clang-format on

    // Clear the screen. This also removes the manufacturer logo, if present.
    system_table->con_out->clear_screen(system_table->con_out);

    auto loaded_image_protocol_guid = EFI::LoadedImageProtocol::guid;
    EFI::LoadedImageProtocol* loaded_image_interface;
    if (auto status = boot_services->handle_protocol(image_handle, &loaded_image_protocol_guid, reinterpret_cast<void**>(&loaded_image_interface)); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get the loaded image protocol");
        return status;
    }

    efi_dbgln(u"Well Hello Friends!");

    test(test2());

    efi_dbgln(u"bss?");
    for (u8 e : bss_test) {
        if (e != 0)
            efi_dbgln(u"bss no worky :^(");
    }

    // FIXME: efibootmgr --unicode doesn't add a NUL terminator.
    efi_dbgln(u"Command line:");
    if (loaded_image_interface->load_options != nullptr && loaded_image_interface->load_options_size != 0)
        efi_dbgln(reinterpret_cast<char16_t const*>(loaded_image_interface->load_options));
    else
        efi_dbgln(u"<empty>");

    auto maybe_kernel_image = load_kernel(loaded_image_interface->device_handle);
    if (maybe_kernel_image.is_error()) {
        efi_dbgln(u"Failed to load the kernel image");
        return maybe_kernel_image.release_error();
    }

    auto* kernel_image = maybe_kernel_image.release_value();

#if ARCH(RISCV64)
    // Get the boot hart ID.
    auto riscv_boot_protocol_guid = EFI::RISCVBootProtocol::guid;
    EFI::RISCVBootProtocol* riscv_boot_protocol = nullptr;

    if (auto status = boot_services->locate_protocol(&riscv_boot_protocol_guid, nullptr, reinterpret_cast<void**>(&riscv_boot_protocol)); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to locate the RISC-V boot protocol!");
        efi_dbgln(u"RISC-V systems that don't support RISCV_EFI_BOOT_PROTOCOL are not supported.");
        return status;
    }

    FlatPtr boot_hart_id = 0;
    if (auto status = riscv_boot_protocol->get_boot_hart_id(riscv_boot_protocol, &boot_hart_id); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get the RISC-V boot hart ID!");
        return status;
    }

    FlatPtr fdt_addr = 0;
    // Get the flattened devicetree from the configuration table.
    for (FlatPtr i = 0; i < system_table->number_of_table_entries; ++i) {
        if (system_table->configuration_table[i].vendor_guid == EFI::DTB_TABLE_GUID) {
            fdt_addr = bit_cast<FlatPtr>(system_table->configuration_table[i].vendor_table);
        }
    }

    if (fdt_addr == 0) {
        efi_dbgln(u"Failed to find the devicetree configuration table!");
        efi_dbgln(u"RISC-V systems without a devicetree UEFI configuration table are not supported.");
        return EFI::Status::LoadError;
    }
#endif

    struct EFIMemoryMap {
        FlatPtr buffer_size = 0;
        FlatPtr memory_map_size = 0;
        EFI::MemoryDescriptor* memory_map = nullptr;
        FlatPtr map_key = 0;
        FlatPtr descriptor_size = 0;
        u32 descriptor_version = 0;
    } efi_memory_map;

    efi_dbgln(u"Getting the EFI memory map");

    // Get the required size for the memory map.
    if (auto status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, nullptr, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version); status != EFI::Status::BufferTooSmall) {
        efi_dbgln(u"Failed to acquire the required size for memory map");
        return status;
    }

    // Make room for 10 additional descriptors in the memory map, so we can reuse the memory map even if the first call to ExitBootServices() fails.
    // We probably shouldn't allocate memory if ExitBootServices() failed, as that might change the memory map again.
    efi_memory_map.memory_map_size += efi_memory_map.descriptor_size * 10;

    // We have to save the size here, as GetMemoryMap() overrides the value pointed to by the MemoryMap argument.
    efi_memory_map.buffer_size = efi_memory_map.memory_map_size;

    if (auto status = boot_services->allocate_pool(EFI::MemoryType::LoaderData, efi_memory_map.buffer_size, reinterpret_cast<void**>(&efi_memory_map.memory_map)); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to allocate memory for the memory map");
        return status;
    }

    if (auto status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, efi_memory_map.memory_map, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get the memory map");
        boot_services->free_pool(efi_memory_map.memory_map);
        return status;
    }

    efi_dbgln(u"Exiting boot services");
    // From now on, we can't use any boot service or device-handle-based protocols anymore, even if ExitBootServices() failed.
    if (auto status = boot_services->exit_boot_services(image_handle, efi_memory_map.map_key); status == EFI::Status::InvalidParameter) {
        // We have to call GetMemoryMap() again, as the memory map changed between GetMemoryMap() and ExitBootServices().
        // Memory allocation services are still allowed to be used if ExitBootServices() failed.
        efi_memory_map.memory_map_size = efi_memory_map.buffer_size;
        if (auto status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, efi_memory_map.memory_map, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version); status != EFI::Status::Success) {
            boot_services->free_pool(efi_memory_map.memory_map);
            return status;
        }

        if (auto status = boot_services->exit_boot_services(image_handle, efi_memory_map.map_key); status != EFI::Status::Success) {
            boot_services->free_pool(efi_memory_map.memory_map);
            return status;
        }
    } else if (status != EFI::Status::Success) {
        return status;
    }

#if ARCH(RISCV64)
    using RISCVEntry = void(FlatPtr boot_hart_id, FlatPtr fdt_phys_addr);
    auto* entry = reinterpret_cast<RISCVEntry*>(kernel_image);

    // The RISC-V kernel requires the MMU to be disabled on entry.
    // We are identity mapped, so we can safely disable it.
    asm volatile("csrw satp, zero");

    // FIXME: Use the UEFI memory map on RISC-V and pass the UEFI command line to the kernel.
    entry(boot_hart_id, fdt_addr);
#else
    (void)kernel_image;
#endif

    halt();
}

}

void __assertion_failed(char const*, char const*, unsigned int, char const*)
{
    Prekernel::halt();
}

// Define some Itanium C++ ABI methods to stop the linker from complaining.
// If we actually call these something has gone horribly wrong
void* __dso_handle __attribute__((visibility("hidden")));
