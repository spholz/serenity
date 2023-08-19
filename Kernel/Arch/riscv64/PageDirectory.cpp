#include <AK/Format.h>

#include <Kernel/API/POSIX/errno.h>
#include <Kernel/Arch/PageDirectory.h>
#include <Kernel/Library/LockRefPtr.h>
#include <Kernel/Sections.h>

namespace Kernel::Memory {

ErrorOr<NonnullLockRefPtr<PageDirectory>> PageDirectory::try_create_for_userspace(Process&)
{
    TODO_RISCV64();
}

LockRefPtr<PageDirectory> PageDirectory::find_current()
{
    TODO_RISCV64();
}

void activate_kernel_page_directory(PageDirectory const& page_directory)
{
    // dbgln("page_directory: {}", page_directory.root_table_paddr());
    FlatPtr const satp_val = (FlatPtr)SatpMode::Sv39 << 60 | (FlatPtr)page_directory.root_table_paddr() >> PADDR_PPN_OFFSET;
    asm volatile(
        "csrw satp, %0\n"
        "sfence.vma\n" ::"r"(satp_val)
        : "memory");
}

void activate_page_directory(PageDirectory const& page_directory, Thread* current_thread)
{
    (void)page_directory;
    (void)current_thread;
    TODO_RISCV64();
}

UNMAP_AFTER_INIT NonnullLockRefPtr<PageDirectory> PageDirectory::must_create_kernel_page_directory()
{
    return adopt_lock_ref_if_nonnull(new (nothrow) PageDirectory).release_nonnull();
}

UNMAP_AFTER_INIT void PageDirectory::allocate_kernel_directory()
{
    dmesgln("MM: boot_pdpt @ {}", boot_pdpt);
    dmesgln("MM: boot_pd0 @ {}", boot_pd0);
    dmesgln("MM: boot_pd_kernel @ {}", boot_pd_kernel);
    m_directory_table = PhysicalPage::create(boot_pdpt, MayReturnToFreeList::No);
    m_directory_pages[0] = PhysicalPage::create(boot_pd0, MayReturnToFreeList::No);
    m_directory_pages[(kernel_mapping_base >> 30) & 0x1ff] = PhysicalPage::create(boot_pd_kernel, MayReturnToFreeList::No);
}

}
