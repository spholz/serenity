/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/AtomicRefCounted.h>
#include <AK/IntrusiveRedBlackTree.h>
#include <AK/RefPtr.h>

#include <Kernel/Forward.h>
#include <Kernel/Locking/Spinlock.h>
#include <Kernel/Memory/PhysicalAddress.h>
#include <Kernel/Memory/PhysicalPage.h>

namespace Kernel::Memory {

// Documentation for RISC-V Virtual Memory:
// The RISC-V Instruction Set Manual, Volume II: Privileged Architecture
// https://github.com/riscv/riscv-isa-manual/releases/download/Priv-v1.12/riscv-privileged-20211203.pdf

// Currently, only the Sv39 (3 level paging) virtual memory system is implemented

// Figure 4.19-4.21
constexpr size_t PAGE_TABLE_SIZE = 0x1000;

constexpr size_t PADDR_PPN_OFFSET = 12;
constexpr size_t VADDR_VPN_OFFSET = 12;
constexpr size_t PTE_PPN_OFFSET = 10;

constexpr size_t PPN_SIZE = 26 + 9 + 9;
constexpr size_t VPN_SIZE = 9 + 9 + 9;

constexpr size_t PPN_MASK = (1LU << PPN_SIZE) - 1;
constexpr size_t PTE_PPN_MASK = PPN_MASK << PTE_PPN_OFFSET;

enum class PageTableEntryFlags {
    Valid = 1 << 0,
    Readable = 1 << 1,
    Writeable = 1 << 2,
    Executable = 1 << 3,
    UserAllowed = 1 << 4,
    Global = 1 << 5,
    Accessed = 1 << 6,
    Dirty = 1 << 7,
};
AK_ENUM_BITWISE_OPERATORS(PageTableEntryFlags);

enum class SatpMode {
    Bare = 0,
    Sv39 = 8,
    Sv48 = 9,
    Sv67 = 10,
};

class PageDirectoryEntry {
public:
    PhysicalPtr page_table_base() const { return (m_raw >> PTE_PPN_OFFSET) << PADDR_PPN_OFFSET; }
    void set_page_table_base([[maybe_unused]] PhysicalPtr value)
    {
        m_raw &= ~PTE_PPN_MASK; // clear old PPN bits
        m_raw |= (value >> PADDR_PPN_OFFSET) << PTE_PPN_OFFSET;
    }

    void clear() { m_raw = 0; }

    bool is_present() const
    {
        return (m_raw & to_underlying(PageTableEntryFlags::Valid)) != 0;
    }
    void set_present(bool b)
    {
        set_bit(PageTableEntryFlags::Valid, b);
    }

    // bool is_user_allowed() const { TODO_RISCV64(); }
    void set_user_allowed(bool)
    {
        // NOOP for RISC-V as non-leaf PTEs don't have this flag
    }

    // bool is_writable() const { TODO_RISCV64(); }
    void set_writable(bool)
    {
        // NOOP for RISC-V as non-leaf PTEs don't have this flag
    }

    // bool is_global() const { TODO_RISCV64(); }
    void set_global(bool)
    {
        // FIXME: global bit doesn't inherit on RISC-V
        // set_bit(PageTableEntryFlags::Global, b);
    }

private:
    void set_bit(PageTableEntryFlags bit, bool value)
    {
        if (value)
            m_raw |= to_underlying(bit);
        else
            m_raw &= ~to_underlying(bit);
    }

    u64 m_raw;
};

class PageTableEntry {
public:
    PhysicalPtr physical_page_base() const { return PhysicalAddress::physical_page_base(m_raw); }
    void set_physical_page_base([[maybe_unused]] PhysicalPtr value)
    {
        m_raw &= ~PTE_PPN_MASK; // clear old PPN bits
        m_raw |= (value >> PADDR_PPN_OFFSET) << PTE_PPN_OFFSET;
    }

    // bool is_present() const { TODO_RISCV64(); }
    void set_present(bool b)
    {
        set_bit(PageTableEntryFlags::Valid, b);
        set_bit(PageTableEntryFlags::Readable, b);
        set_bit(PageTableEntryFlags::Accessed, b);
        set_bit(PageTableEntryFlags::Dirty, b);

        // FIXME: dont set all permissions
        set_bit(PageTableEntryFlags::Writeable, b);
        set_bit(PageTableEntryFlags::Executable, b);
    }

    // bool is_user_allowed() const { TODO_RISCV64(); }
    void set_user_allowed(bool b)
    {
        set_bit(PageTableEntryFlags::UserAllowed, b);
    }

    bool is_writable() const
    {
        return (m_raw & to_underlying(PageTableEntryFlags::Writeable)) != 0;
    }
    void set_writable(bool)
    {
        // Only W bit set is reserved (Table 4.5)
        // set_bit(PageTableEntryFlags::Readable, b);

        // set_bit(PageTableEntryFlags::Writeable, b);
    }

    // bool is_cache_disabled() const { TODO_RISCV64(); }
    void set_cache_disabled(bool)
    {
        // FIXME: what to do here?
    }

    // bool is_global() const { TODO_RISCV64(); }
    void set_global(bool)
    {
        // set_bit(PageTableEntryFlags::Global, b);
    }

    // bool is_execute_disabled() const { TODO_RISCV64(); }
    void set_execute_disabled(bool)
    {
        // set_bit(PageTableEntryFlags::Executable, !b);
    }

    // bool is_pat() const { TODO_RISCV64(); }
    void set_pat(bool)
    {
        // Processor::has_pat() returns false
    }

    bool is_null() const { return m_raw == 0; }
    void clear() { m_raw = 0; }

private:
    void set_bit(PageTableEntryFlags bit, bool value)
    {
        if (value)
            m_raw |= to_underlying(bit);
        else
            m_raw &= ~to_underlying(bit);
    }

    u64 m_raw;
};

class PageDirectoryPointerTable {
public:
    PageDirectoryEntry* directory(size_t index)
    {
        VERIFY(index <= (NumericLimits<size_t>::max() << 30));
        return (PageDirectoryEntry*)(PhysicalAddress::physical_page_base(raw[index]));
    }

    u64 raw[512];
};

class PageDirectory final : public AtomicRefCounted<PageDirectory> {
    friend class MemoryManager;

public:
    static ErrorOr<NonnullLockRefPtr<PageDirectory>> try_create_for_userspace(Process&);
    static NonnullLockRefPtr<PageDirectory> must_create_kernel_page_directory();
    static LockRefPtr<PageDirectory> find_current();

    void allocate_kernel_directory();

    FlatPtr satp() const
    {
        return (FlatPtr)SatpMode::Sv39 << 60 | (FlatPtr)m_directory_table->paddr().get() >> PADDR_PPN_OFFSET;
    }

    Process* process() { return m_process; }

    RecursiveSpinlock<LockRank::None>& get_lock() { return m_lock; }

    // This has to be public to let the global singleton access the member pointer
    IntrusiveRedBlackTreeNode<FlatPtr, PageDirectory, RawPtr<PageDirectory>> m_tree_node;

private:
    static void register_page_directory(PageDirectory* directory);
    static void deregister_page_directory(PageDirectory* directory);

    Process* m_process { nullptr };
    RefPtr<PhysicalPage> m_directory_table;
    RefPtr<PhysicalPage> m_directory_pages[512];
    RecursiveSpinlock<LockRank::None> m_lock {};
};

void activate_kernel_page_directory([[maybe_unused]] PageDirectory const& pgd);
void activate_page_directory([[maybe_unused]] PageDirectory const& pgd, [[maybe_unused]] Thread* current_thread);

}
