/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Types.h>

#include <Kernel/Arch/riscv64/ASM_wrapper.h>
#include <Kernel/Arch/riscv64/CPU.h>
#include <Kernel/Arch/riscv64/PageDirectory.h>
#include <Kernel/Sections.h>

// These come from the linker script
extern u8 page_tables_phys_start[];
extern u8 page_tables_phys_end[];
extern u8 start_of_kernel_image[];
extern u8 end_of_kernel_image[];

namespace Kernel::Memory {

class PageBumpAllocator {
public:
    PageBumpAllocator(u64* start, u64* end)
        : m_start(start)
        , m_end(end)
        , m_current(start)
    {
        if (m_start >= m_end) {
            panic_without_mmu("Invalid memory range passed to PageBumpAllocator"sv);
        }
        if ((FlatPtr)m_start % PAGE_TABLE_SIZE != 0 || (FlatPtr)m_end % PAGE_TABLE_SIZE != 0) {
            panic_without_mmu("Memory range passed into PageBumpAllocator not aligned to PAGE_TABLE_SIZE"sv);
        }
    }

    u64* take_page()
    {
        if (m_current == m_end) {
            panic_without_mmu("Prekernel pagetable memory exhausted"sv);
        }

        u64* page = m_current;
        m_current += (PAGE_TABLE_SIZE / sizeof(FlatPtr));

        // dbgln("    page alloc: {:p}", page);

        zero_page(page);
        return page;
    }

private:
    void zero_page(u64* page)
    {
        // Memset all page table memory to zero
        for (u64* p = page; p < page + (PAGE_TABLE_SIZE / sizeof(u64)); p++) {
            *p = 0;
        }
    }

    u64 const* m_start;
    u64 const* m_end;
    u64* m_current;
};

static FlatPtr calculate_physical_to_link_time_address_offset()
{
    // TODO: is there a better way to obtain the runtime address?
    FlatPtr physical_address;
    FlatPtr link_time_address;
    asm volatile(
        ".option push\n"
        ".option norvc\n"
        "1: jal %[physical_address], 3f\n"
        "2: .dword 1b\n"
        "3: ld %[link_time_address], 2b\n"
        ".option pop\n"
        : [physical_address] "=r"(physical_address),
        [link_time_address] "=r"(link_time_address));

    // subtract 4 from physical_address as (pc+4) is stored into the register by a jal instruction
    return link_time_address - (physical_address - 4);
}

template<typename T>
inline T* adjust_by_mapping_base(T* ptr)
{
    return (T*)((FlatPtr)ptr - calculate_physical_to_link_time_address_offset());
}

static bool page_table_entry_valid(u64 entry)
{
    return (entry & to_underlying(PageTableEntryFlags::Valid)) != 0;
}

static u64* insert_page_table(PageBumpAllocator& allocator, u64* root_table, VirtualAddress virtual_addr)
{
    size_t vpn_1 = (virtual_addr.get() >> VPN_1_OFFSET) & PAGE_TABLE_INDEX_MASK;
    size_t vpn_2 = (virtual_addr.get() >> VPN_2_OFFSET) & PAGE_TABLE_INDEX_MASK;

    u64* level0_table = root_table;

    if (!page_table_entry_valid(level0_table[vpn_2])) {
        level0_table[vpn_2] = ((FlatPtr)allocator.take_page() >> PADDR_PPN_OFFSET) << PTE_PPN_OFFSET;
        level0_table[vpn_2] |= to_underlying(PageTableEntryFlags::Valid);
    }

    u64* level1_table = (u64*)((level0_table[vpn_2] >> PTE_PPN_OFFSET) << PADDR_PPN_OFFSET);

    if (!page_table_entry_valid(level1_table[vpn_1])) {
        level1_table[vpn_1] = ((FlatPtr)allocator.take_page() >> PADDR_PPN_OFFSET) << PTE_PPN_OFFSET;
        level1_table[vpn_1] |= to_underlying(PageTableEntryFlags::Valid);
    }

    return (u64*)((level1_table[vpn_1] >> PTE_PPN_OFFSET) << PADDR_PPN_OFFSET);
}

static u64* get_page_directory(u64* root_table, VirtualAddress virtual_addr)
{
    size_t vpn_2 = (virtual_addr.get() >> VPN_2_OFFSET) & PAGE_TABLE_INDEX_MASK;

    u64* level0_table = root_table;

    if (!page_table_entry_valid(level0_table[vpn_2]))
        return nullptr;

    return (u64*)((level0_table[vpn_2] >> PTE_PPN_OFFSET) << PADDR_PPN_OFFSET);
}

static void insert_entries_for_memory_range(PageBumpAllocator& allocator, u64* root_table, VirtualAddress start, VirtualAddress end, PhysicalAddress paddr, PageTableEntryFlags flags)
{
    // dbgln("start: {}, end: {}, paddr: {}", start, end, paddr);

    // Not very efficient, but simple and it works.
    for (VirtualAddress vaddr = start; vaddr < end;) {
        u64* level2_table = insert_page_table(allocator, root_table, vaddr);

        size_t vpn_0 = (vaddr.get() >> PADDR_PPN_OFFSET) & PAGE_TABLE_INDEX_MASK;
        level2_table[vpn_0] = (paddr.get() >> PADDR_PPN_OFFSET) << PTE_PPN_OFFSET;
        level2_table[vpn_0] |= to_underlying(PageTableEntryFlags::Valid | PageTableEntryFlags::Accessed | PageTableEntryFlags::Dirty | flags);

        vaddr = vaddr.offset(PAGE_TABLE_SIZE);
        paddr = paddr.offset(PAGE_TABLE_SIZE);
    }
}

static void setup_quickmap_page_table(PageBumpAllocator& allocator, u64* root_table)
{
    auto kernel_pt1024_base = VirtualAddress(*adjust_by_mapping_base(&kernel_mapping_base) + KERNEL_PT1024_OFFSET);

    auto quickmap_page_table = PhysicalAddress((PhysicalPtr)insert_page_table(allocator, root_table, kernel_pt1024_base));
    *adjust_by_mapping_base(&boot_pd_kernel_pt1023) = (PageTableEntry*)quickmap_page_table.offset(calculate_physical_to_link_time_address_offset()).get();
}

static void build_mappings(PageBumpAllocator& allocator, u64* root_table)
{
    // Align the identity mapping of the kernel image to 2 MiB, the rest of the memory is initially not mapped.
    auto start_of_kernel_range = VirtualAddress((FlatPtr)start_of_kernel_image & ~(FlatPtr)0x1fffff).offset(-512 * 1024L);
    auto end_of_kernel_range = VirtualAddress(((FlatPtr)end_of_kernel_image & ~(FlatPtr)0x1fffff) + 0x200000 - 1);

    auto start_of_physical_kernel_range = PhysicalAddress(start_of_kernel_range.get()).offset(-calculate_physical_to_link_time_address_offset());

    // FIXME: dont map everything RWX

    // Insert identity mappings
    insert_entries_for_memory_range(allocator, root_table, start_of_kernel_range.offset(-calculate_physical_to_link_time_address_offset()), end_of_kernel_range.offset(-calculate_physical_to_link_time_address_offset()), start_of_physical_kernel_range, PageTableEntryFlags::Readable | PageTableEntryFlags::Executable | PageTableEntryFlags::Writeable);

    // Map kernel into high virtual memory
    insert_entries_for_memory_range(allocator, root_table, start_of_kernel_range, end_of_kernel_range, start_of_physical_kernel_range, PageTableEntryFlags::Readable | PageTableEntryFlags::Executable | PageTableEntryFlags::Writeable);
}

static void activate_mmu(u64 const* root_table)
{
    RISCV64::Satp satp = {
        .PPN = (FlatPtr)root_table >> PADDR_PPN_OFFSET,
        .ASID = 0,
        .MODE = RISCV64::Satp::Mode::Sv39,
    };

    RISCV64::Satp::write(satp);
    Processor::flush_entire_tlb_local();
}

static void setup_kernel_page_directory(u64* root_table)
{
    auto kernel_page_directory = (PhysicalPtr)get_page_directory(root_table, VirtualAddress { *adjust_by_mapping_base(&kernel_mapping_base) });
    if (kernel_page_directory == 0)
        panic_without_mmu("Could not find kernel page directory!"sv);

    *adjust_by_mapping_base(&boot_pd_kernel) = PhysicalAddress(kernel_page_directory);

    // NOTE: There is no level 4 table in Sv39
    *adjust_by_mapping_base(&boot_pml4t) = PhysicalAddress(0);

    *adjust_by_mapping_base(&boot_pdpt) = PhysicalAddress((PhysicalPtr)root_table);
}

void init_page_tables()
{
    // TODO: verify satp.MODE == Bare
    *adjust_by_mapping_base(&physical_to_virtual_offset) = calculate_physical_to_link_time_address_offset();
    *adjust_by_mapping_base(&kernel_mapping_base) = KERNEL_MAPPING_BASE;
    *adjust_by_mapping_base(&kernel_load_base) = KERNEL_MAPPING_BASE;

    PageBumpAllocator allocator(adjust_by_mapping_base((u64*)page_tables_phys_start), adjust_by_mapping_base((u64*)page_tables_phys_end));
    auto* root_table = allocator.take_page();
    build_mappings(allocator, root_table);
    setup_quickmap_page_table(allocator, root_table);
    setup_kernel_page_directory(root_table);

    activate_mmu(root_table);
}

}
