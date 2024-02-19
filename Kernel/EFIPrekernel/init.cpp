/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/EFIPrekernel/EFI/EFI.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/LoadedImage.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/MediaAccess.h>
#include <Kernel/EFIPrekernel/EFI/Protocols/RISCVBootProtocol.h>
#include <Kernel/EFIPrekernel/EFI/SystemTable.h>
#include <LibELF/ELFABI.h>
#include <LibELF/Relocation.h>

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

#if ARCH(RISCV64)
    char16_t const kernel_file_name[] = u"Kernel.bin";
#else
    char16_t const kernel_file_name[] = u"Kernel";
#endif

    EFI::FileProtocol* file;
    if (auto status = volume->open(volume, &file, const_cast<char16_t*>(kernel_file_name), EFI::FileOpenMode::Read, EFI::FileAttribute::None); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to open kernel image");
        return status;
    }

    auto file_info_guid = EFI::FileInfo::guid;
    u8 info_buffer[sizeof(EFI::FileInfo) + sizeof(kernel_file_name)];
    FlatPtr info_size = sizeof(info_buffer);
    if (auto status = file->get_info(file, &file_info_guid, &info_size, &info_buffer); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get info for kernel image");
        return status;
    }

    auto const* info = reinterpret_cast<EFI::FileInfo const*>(info_buffer);
    FlatPtr kernel_size = info->file_size;

#if ARCH(RISCV64)
    // The RISC-V kernel uses some memory after the kernel image for the stack and initial page tables.
    kernel_size += 12 * MiB;
#endif

    FlatPtr kernel_page_count = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;

    EFI::PhysicalAddress kernel_image_address = 0;
    if (auto status = s_system_table->boot_services->allocate_pages(EFI::AllocateType::AnyPages, EFI::MemoryType::LoaderData, kernel_page_count, &kernel_image_address); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to allocate pages for kernel image");
        return status;
    }

    auto* kernel_image = bit_cast<void*>(kernel_image_address);

    efi_dbgln(u"Loading kernel image...");
    if (auto status = file->read(file, &kernel_size, kernel_image); status != EFI::Status::Success) {
        efi_dbgln(u"Failed to read kernel image");
        return status;
    }
    efi_dbgln(u"Done");

    return kernel_image;
}

// This gets called from boot.S.
extern "C" void perform_relative_relocations(FlatPtr base_address, FlatPtr dynamic_section_addr, EFI::SystemTable* system_table);
extern "C" void perform_relative_relocations(FlatPtr base_address, FlatPtr dynamic_section_addr, EFI::SystemTable* system_table)
{
    if (!ELF::perform_relative_relocations(base_address, dynamic_section_addr))
        system_table->con_out->output_string(system_table->con_out, const_cast<char16_t*>(u"Failed to self-relocate\r\n"));
}

extern "C" EFIAPI EFI::Status init(EFI::Handle image_handle, EFI::SystemTable* system_table);
extern "C" EFIAPI EFI::Status init(EFI::Handle image_handle, EFI::SystemTable* system_table)
{
    // FIXME: Clean up memory/resources before returning errors
    // FIXME: Clean up error handling code

    auto* boot_services = system_table->boot_services;
    auto loaded_image_protocol_guid = EFI::LoadedImageProtocol::guid;
    EFI::LoadedImageProtocol* loaded_image_interface;

    if (system_table->hdr.signature != EFI::SystemTable::signature || system_table->hdr.revision < ((1 << 16) | (10)))
        return EFI::Status::Unsupported;

    EFI::Status status = boot_services->handle_protocol(image_handle, &loaded_image_protocol_guid, reinterpret_cast<void**>(&loaded_image_interface));
    if (status != EFI::Status::Success) {
        // We can't safely use efi_dbgln yet, as it would use the global system table variable.
        system_table->con_out->output_string(system_table->con_out, const_cast<char16_t*>(u"Failed to get loaded image protocol\r\n"));
        return status;
    }

    s_image_handle = image_handle;
    s_system_table = system_table;

    efi_dbgln(u"Hello, world!");

    efi_dbgln(u"Command line:");
    efi_dbgln(reinterpret_cast<char16_t const*>(loaded_image_interface->load_options));

    auto maybe_kernel_image = load_kernel(loaded_image_interface->device_handle);
    if (maybe_kernel_image.is_error()) {
        efi_dbgln(u"Failed to load kernel");
        return maybe_kernel_image.release_error();
    }

    auto* kernel_image = maybe_kernel_image.release_value();

#if ARCH(RISCV64)
    // Get the boot hart ID.
    auto riscv_boot_protocol_guid = EFI::RISCVBootProtocol::guid;
    EFI::RISCVBootProtocol* riscv_boot_protocol = nullptr;

    status = boot_services->locate_protocol(&riscv_boot_protocol_guid, nullptr, reinterpret_cast<void**>(&riscv_boot_protocol));
    if (status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get RISC-V boot protocol");
        efi_dbgln(u"RISC-V systems that don't support RISCV_EFI_BOOT_PROTOCOL are not supported.");
        return status;
    }

    FlatPtr boot_hart_id = 0;
    status = riscv_boot_protocol->get_boot_hart_id(riscv_boot_protocol, &boot_hart_id);
    if (status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get RISC-V boot hart ID");
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

    efi_dbgln(u"Getting memory map");

    // Get the required size for the memory map.
    status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, nullptr, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version);
    if (status != EFI::Status::BufferTooSmall) {
        efi_dbgln(u"Failed to acquire required size for memory map");
        return status;
    }

    // Make room for 10 additional descriptors in the memory map, so we can reuse the memory map even if the first call to ExitBootServices() fails.
    // We probably shouldn't allocate memory if ExitBootServices() failed, as that might change the memory map again.
    efi_memory_map.memory_map_size += efi_memory_map.descriptor_size * 10;

    // We have to save the size here, as GetMemoryMap() overrides the value pointed to by the MemoryMap argument.
    efi_memory_map.buffer_size = efi_memory_map.memory_map_size;

    status = boot_services->allocate_pool(EFI::MemoryType::LoaderData, efi_memory_map.buffer_size, reinterpret_cast<void**>(&efi_memory_map.memory_map));
    if (status != EFI::Status::Success) {
        efi_dbgln(u"Failed to allocate memory for memory map");
        return status;
    }

    status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, efi_memory_map.memory_map, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version);
    if (status != EFI::Status::Success) {
        efi_dbgln(u"Failed to get memory map");
        boot_services->free_pool(efi_memory_map.memory_map);
        return status;
    }

    efi_dbgln(u"Exiting boot services");
    status = boot_services->exit_boot_services(image_handle, efi_memory_map.map_key);
    // From now on, we can't use any boot service or device-handle-based protocols anymore, even if ExitBootServices() failed.
    if (status == EFI::Status::InvalidParameter) {
        // We have to call GetMemoryMap() again, as the memory map changed between GetMemoryMap() and ExitBootServices().
        // Memory allocation services are still allowed to be used if ExitBootServices() failed.
        efi_memory_map.memory_map_size = efi_memory_map.buffer_size;
        status = boot_services->get_memory_map(&efi_memory_map.memory_map_size, efi_memory_map.memory_map, &efi_memory_map.map_key, &efi_memory_map.descriptor_size, &efi_memory_map.descriptor_version);
        if (status != EFI::Status::Success) {
            boot_services->free_pool(efi_memory_map.memory_map);
            return status;
        }

        status = boot_services->exit_boot_services(image_handle, efi_memory_map.map_key);
        if (status != EFI::Status::Success) {
            boot_services->free_pool(efi_memory_map.memory_map);
            return status;
        }
    } else if (status != EFI::Status::Success) {
        return status;
    }

#if ARCH(AARCH64)
    using AArch64Entry = void();
    auto* entry = reinterpret_cast<AArch64Entry*>(kernel_image);
    entry();
#elif ARCH(RISCV64)
    using RISCVEntry = void(FlatPtr boot_hart_id, FlatPtr fdt_phys_addr);
    auto* entry = reinterpret_cast<RISCVEntry*>(kernel_image);

    // The kernel requires the MMU to be disabled.
    // We are identity mapped, so we can safely disable it.
    asm volatile("csrw satp, zero");

    entry(boot_hart_id, fdt_addr);
#else
    (void)kernel_image;
#endif

    halt();
}

}

// Dummy .reloc section with 0 fixups so objcopy doesn't set the IMAGE_FILE_RELOCS_STRIPPED flag.
// UEFI firmwares don't seem to like images with that flag.
// https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#the-reloc-section-image-only
[[gnu::section(".reloc")]] u32 dummy_reloc_section[2] = {
    0,                           // Page RVA
    sizeof(dummy_reloc_section), // Block Size
};

void __assertion_failed(char const*, char const*, unsigned int, char const*)
{
    Prekernel::halt();
}
