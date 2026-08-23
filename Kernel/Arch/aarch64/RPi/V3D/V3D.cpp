/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/V3D.h>
#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/MemoryFences.h>
#include <Kernel/Arch/aarch64/ASM_wrapper.h>
#include <Kernel/Arch/aarch64/RPi/V3D/GPU3DDevice.h>
#include <Kernel/Arch/aarch64/RPi/V3D/Registers.h>
#include <Kernel/Arch/aarch64/RPi/V3D/V3D.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>
#include <Kernel/Memory/MemoryManager.h>

namespace Kernel::RPi::V3D {

static void dump_hub_registers(HubRegisters const volatile& registers)
{
    dbgln("V3D Hub Registers:");
    dbgln("  UIFCFG: {:#08x}", (u32)registers.uifcfg);
    dbgln("  IDENT0: {:#08x}", (u32)registers.identification_0);
    dbgln("  IDENT1: {:#08x}", (u32)registers.identification_1);
    dbgln("  IDENT2: {:#08x}", (u32)registers.identification_2);
    dbgln("  IDENT3: {:#08x}", (u32)registers.identification_3);
    dbgln("  Interrupt status: {:#08x}", (u32)registers.interrupt_status);
    dbgln("  Interrupt mask: {:#08x}", (u32)registers.interrupt_mask);
    dbgln("MMU 0:");
    dbgln("  MMUC control: {:#08x}", (u32)registers.mmu_0.mmu_cache_control);
    dbgln("  Control: {:#08x}", (u32)registers.mmu_0.control);
    dbgln("  Page table base paddr: {:#08x}", (u32)registers.mmu_0.page_table_base_page_index);
    dbgln("  Fault AXI ID: {:#08x}", (u32)registers.mmu_0.fault_axi_id);
    dbgln("  Illegal vaddr target paddr: {:#08x}", (u32)registers.mmu_0.illegal_vaddr_target_paddr);
    dbgln("  Fault vaddr: {:#08x}", (u32)registers.mmu_0.fault_vaddr);
    dbgln("  Debug info: {:#08x}", (u32)registers.mmu_0.debug_info);
}

static void dump_core_registers(CoreRegisters const volatile& registers)
{
    dbgln("V3D Core Registers:");
    dbgln("  IDENT0: {:#08x}", (u32)registers.identification_0);
    dbgln("  IDENT1: {:#08x}", (u32)registers.identification_1);
    dbgln("  IDENT2: {:#08x}", (u32)registers.identification_2);
    dbgln("  MISCCFG: {:#08x}", (u32)registers.misccfg);
    dbgln("  INTSTS: {:#08x}", (u32)registers.interrupt_status);
    dbgln("  PCS: {:#08x}", (u32)registers.control_list_executor.pipeline_control_and_status);
    dbgln("  BFC: {:#08x}", (u32)registers.control_list_executor.binning_mode_flush_count);
    dbgln("  RFC: {:#08x}", (u32)registers.control_list_executor.rendering_mode_flush_count);
    dbgln("  BPCA: {:#08x}", (u32)registers.current_address_of_binning_memory_pool);
    dbgln("  BPCS: {:#08x}", (u32)registers.remaining_size_of_binning_memory_pool);
    dbgln("  BPOA: {:#08x}", (u32)registers.address_of_overspill_binning_memory_block);
    dbgln("  BPOS: {:#08x}", (u32)registers.size_of_overspill_binning_memory_block);
    dbgln("  FDBGO: {:#08x}", (u32)registers.fep_overrun_error_signals);
    dbgln("  FDBGB: {:#08x}", (u32)registers.fep_interface_ready_and_stall_signals__fep_busy_signals);
    dbgln("  FDBGR: {:#08x}", (u32)registers.fep_interface_ready_signals);
    dbgln("  FDBGS: {:#08x}", (u32)registers.fep_internal_stall_input_signals);
    dbgln("  ERRSTAT: {:#08x}", (u32)registers.miscellaneous_error_signals);
    dbgln("  Thread 0:");
    dbgln("    CT0CS: {:#08x}", (u32)registers.control_list_executor.thread_0_control_and_status);
    dbgln("    CT0EA: {:#08x}", (u32)registers.control_list_executor.thread_0_end_address);
    dbgln("    CT0CA: {:#08x}", (u32)registers.control_list_executor.thread_0_current_address);
    dbgln("    CT0RA: {:#08x}", (u32)registers.control_list_executor.thread_0_return_address);
    dbgln("    CT0LC: {:#08x}", (u32)registers.control_list_executor.thread_0_list_counter);
    dbgln("    CT0PC: {:#08x}", (u32)registers.control_list_executor.thread_0_primitive_list_counter);
    dbgln("    CT0QTS: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_state_data_array_address);
    dbgln("    CT0QBA: {:#08x}", (u32)registers.control_list_executor.thread_0_control_list_start_address);
    dbgln("    CT0QEA: {:#08x}", (u32)registers.control_list_executor.thread_0_control_list_end_address);
    dbgln("    CT0QMA: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_allocation_memory_address);
    dbgln("    CT0QMS: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_allocation_memory_size);
    dbgln("  Thread 1:");
    dbgln("    CT1CS: {:#08x}", (u32)registers.control_list_executor.thread_1_control_and_status);
    dbgln("    CT1EA: {:#08x}", (u32)registers.control_list_executor.thread_1_end_address);
    dbgln("    CT1CA: {:#08x}", (u32)registers.control_list_executor.thread_1_current_address);
    dbgln("    CT1RA: {:#08x}", (u32)registers.control_list_executor.thread_1_return_address);
    dbgln("    CT1LC: {:#08x}", (u32)registers.control_list_executor.thread_1_list_counter);
    dbgln("    CT1PC: {:#08x}", (u32)registers.control_list_executor.thread_1_primitive_list_counter);
    dbgln("    CT1QBA: {:#08x}", (u32)registers.control_list_executor.thread_1_control_list_start_address);
    dbgln("    CT1QEA: {:#08x}", (u32)registers.control_list_executor.thread_1_control_list_end_address);
}

ErrorOr<NonnullRefPtr<V3D>> V3D::create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource, InterruptNumber hub_interrupt_number, Optional<InterruptNumber> core_interrupt_number)
{
    if (hub_registers_resource.size < sizeof(HubRegisters))
        return EINVAL;

    if (core_0_registers_resource.size < sizeof(CoreRegisters))
        return EINVAL;

    auto hub_registers = TRY(Memory::map_typed_writable<HubRegisters volatile>(hub_registers_resource.paddr));
    auto core_0_registers = TRY(Memory::map_typed_writable<CoreRegisters volatile>(core_0_registers_resource.paddr));

    auto v3d = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) V3D(move(hub_registers), move(core_0_registers), hub_interrupt_number, core_interrupt_number)));
    TRY(v3d->initialize());

    return v3d;
}

void V3D::map_buffer(PageTable& page_table, GPUVirtualAddress gpu_vaddr, Memory::VMObject const& vmobject)
{
    page_table.insert_entries_for_buffer({}, gpu_vaddr, vmobject);
}

void V3D::unmap_buffer(PageTable& page_table, GPUVirtualAddress gpu_vaddr, Memory::VMObject const& vmobject)
{
    page_table.remove_entries_for_buffer({}, gpu_vaddr, vmobject);
    flush_mmu_cache_and_tlb();
}

ErrorOr<void> V3D::submit_job(PageTable const& page_table, V3DJob const& job)
{
    // FIXME: Make job submission asynchrnous. This requires some userspace API to wait until the job is finished.
    //        Currently, we just use a Mutex to ensure that only one thread can run a job at a time.
    //        This thread will be blocked until the job is finished.
    //        Once we make job submission asynchronous, we need to ensure that the Context and Buffers used by this job
    //        stay alive until this job is finished (maybe by using `RefPtr`s?).

    MutexLocker locker { m_job_mutex };

    // Ensure that the bottom bits are 0 so we can set the enable bit correctly.
    if ((job.tile_state_data_array_base_address % 4096) != 0)
        return EINVAL;

    activate_page_table(page_table);

    // dbgln("Registers before binning job submission:");
    // dump_hub_registers(*m_hub_registers);
    // dump_core_registers(*m_core_0_registers);

    flush_caches();

    full_memory_fence();

    m_core_0_registers->control_list_executor.thread_0_tile_allocation_memory_address = job.tile_allocation_memory_base_address;
    m_core_0_registers->control_list_executor.thread_0_tile_allocation_memory_size = job.tile_allocation_memory_size;
    m_core_0_registers->control_list_executor.thread_0_tile_state_data_array_address = job.tile_state_data_array_base_address | 1 << 1;

    m_core_0_registers->control_list_executor.thread_0_control_list_start_address = job.binning_control_list_address;
    m_core_0_registers->control_list_executor.thread_0_control_list_end_address = job.binning_control_list_address + job.binning_control_list_size; // XXX: Checked<>::add?

    // FIXME: Cancel the jobs if wait_until returns an error (due to EINTR etc.)
    auto binning_job_wait_result = m_current_binning_job_finished_wait_queue.wait_until(m_current_binning_job_finished, [](bool& job_finished) {
        if (!job_finished)
            return false;
        job_finished = false;
        return true;
    });

    if (binning_job_wait_result.is_error() || m_mmu_faulted.with([](auto mmu_faulted) { return mmu_faulted; })) {
        // VERIFY(binning_job_wait_result.error().code() == EINTR);
        dbgln("Binning job was interrupted!");
        dump_hub_registers(*m_hub_registers);
        dump_core_registers(*m_core_0_registers);

        dbgln("Attempting to halt thread 0");
        m_core_0_registers->control_list_executor.thread_0_control_and_status = 1 << 5;

        if (binning_job_wait_result.is_error())
            return binning_job_wait_result.release_error();

        return EIO;
    }

    // auto binning_mode_flush_count = static_cast<u8>(m_core_0_registers->control_list_executor.binning_mode_flush_count);

    // dbgln("Binning mode flush count: {}", binning_mode_flush_count);

    // dbgln("Registers after binning job / before rendering job submission:");
    // dump_hub_registers(*m_hub_registers);
    // dump_core_registers(*m_core_0_registers);

    flush_caches();

    m_core_0_registers->control_list_executor.thread_1_control_list_start_address = job.rendering_control_list_address;
    m_core_0_registers->control_list_executor.thread_1_control_list_end_address = job.rendering_control_list_address + job.rendering_control_list_size; // XXX: Checked<>::add?

    auto render_job_wait_result = m_current_render_job_finished_wait_queue.wait_until(m_current_render_job_finished, [](bool& job_finished) {
        if (!job_finished)
            return false;
        job_finished = false;
        return true;
    });

    if (render_job_wait_result.is_error() || m_mmu_faulted.with([](auto mmu_faulted) { return mmu_faulted; })) {
        // VERIFY(render_job_wait_result.error().code() == EINTR);
        dbgln("Render job was interrupted!");
        dump_hub_registers(*m_hub_registers);
        dump_core_registers(*m_core_0_registers);

        dbgln("Attempting to halt thread 1");
        m_core_0_registers->control_list_executor.thread_1_control_and_status = 1 << 5;

        if (binning_job_wait_result.is_error())
            return render_job_wait_result.release_error();

        return EIO;
    }

    // auto rendering_mode_flush_count = static_cast<u8>(m_core_0_registers->control_list_executor.rendering_mode_flush_count);

    // dbgln("Rendering mode flush count: {}", rendering_mode_flush_count);

    // dbgln("Registers after rendering job submission:");
    // dump_hub_registers(*m_hub_registers);
    // dump_core_registers(*m_core_0_registers);

    return {};
}

V3D::V3D(Memory::TypedMapping<HubRegisters volatile> hub_registers, Memory::TypedMapping<CoreRegisters volatile> core_0_registers, InterruptNumber hub_interrupt_number, Optional<InterruptNumber> core_interrupt_number)
    : m_hub_registers(move(hub_registers))
    , m_core_0_registers(move(core_0_registers))
    , m_hub_interrupt_handler(*this, hub_interrupt_number)
{
    full_memory_fence(); // Ensure zeroing is visible.

    if (core_interrupt_number.has_value())
        m_core_interrupt_handler.emplace(*this, core_interrupt_number.value());
}

ErrorOr<void> V3D::initialize()
{
    dbgln("Registers before initialize:");
    dump_hub_registers(*m_hub_registers);
    dump_core_registers(*m_core_0_registers);

    m_3d_device = TRY(GPU3DDevice::create(*this));

    full_memory_fence();

    m_illegal_vaddr_target_page = TRY(MM.allocate_physical_page(Memory::MemoryManager::ShouldZeroFill::Yes, nullptr, Memory::MemoryType::NonCacheable));

    m_hub_registers->mmu_0.illegal_vaddr_target_paddr = (m_illegal_vaddr_target_page->paddr().get() >> 12) | (1u << 31);
    m_hub_registers->mmu_0.control = HubRegisters::MMUControl::Enable
        | HubRegisters::MMUControl::WriteViolationInterrupt
        | HubRegisters::MMUControl::WriteViolationAbort
        | HubRegisters::MMUControl::InvalidPageTableEnable
        | HubRegisters::MMUControl::InvalidPageTableInterrupt
        | HubRegisters::MMUControl::InvalidPageTableAbort
        | HubRegisters::MMUControl::CapExceededInterrupt
        | HubRegisters::MMUControl::CapExceededAbort;

    flush_mmu_cache_and_tlb();

    m_hub_registers->interrupt_mask_set = ~(HubRegisters::Interrupt::MMUCapExceeded
        | HubRegisters::Interrupt::MMUPageTableInvalid
        | HubRegisters::Interrupt::MMUWriteViolation);

    m_hub_registers->interrupt_mask_clear = HubRegisters::Interrupt::MMUCapExceeded
        | HubRegisters::Interrupt::MMUPageTableInvalid
        | HubRegisters::Interrupt::MMUWriteViolation;

    m_core_0_registers->interrupt_mask_set = ~(CoreRegisters::Interrupt::RenderModeFrameDone
        | CoreRegisters::Interrupt::BinningModeFlushDone
        | CoreRegisters::Interrupt::BinnerOutOfMemory
        | CoreRegisters::Interrupt::BinnerOverspillMemoryInUse);

    m_core_0_registers->interrupt_mask_clear = CoreRegisters::Interrupt::RenderModeFrameDone
        | CoreRegisters::Interrupt::BinningModeFlushDone
        | CoreRegisters::Interrupt::BinnerOutOfMemory
        | CoreRegisters::Interrupt::BinnerOverspillMemoryInUse;

    dbgln("Registers after initialize:");
    dump_hub_registers(*m_hub_registers);
    dump_core_registers(*m_core_0_registers);

    return {};
}

void V3D::flush_mmu_cache_and_tlb()
{
    m_hub_registers->mmu_0.mmu_cache_control = HubRegisters::MMUCacheControl::Enable | HubRegisters::MMUCacheControl::Flush;
    while (has_flag(m_hub_registers->mmu_0.mmu_cache_control, HubRegisters::MMUCacheControl::Flushing))
        Processor::wait_check();

    m_hub_registers->mmu_0.control |= HubRegisters::MMUControl::TLBClear;
    while (has_flag(m_hub_registers->mmu_0.control, HubRegisters::MMUControl::TLBClearing))
        Processor::wait_check();
}

void V3D::flush_caches()
{
    m_core_0_registers->texture_cache_flush_start_addr = 0;
    m_core_0_registers->texture_cache_flush_end_addr = 0xffff'ffff;
    m_core_0_registers->texture_cache_control = 1;

    m_core_0_registers->slices_cache_control = 0xffff'ffff;
}

void V3D::activate_page_table(PageTable const& page_table)
{
    auto page_table_physical_page_index = page_table.physical_address().get() >> 12;

    if (m_hub_registers->mmu_0.page_table_base_page_index == page_table_physical_page_index) {
        // Already active, nothing to do.
        return;
    }

    m_hub_registers->mmu_0.page_table_base_page_index = page_table_physical_page_index;
    flush_mmu_cache_and_tlb();
}

bool V3D::handle_interrupt()
{
    auto hub_interrupts = m_hub_registers->interrupt_status;

    m_hub_registers->interrupt_clear_pending = hub_interrupts;

    if (has_any_flag(hub_interrupts, HubRegisters::Interrupt::MMUCapExceeded | HubRegisters::Interrupt::MMUPageTableInvalid | HubRegisters::Interrupt::MMUWriteViolation)) {
        dbgln("V3D: Page fault!");

        if (has_flag(hub_interrupts, HubRegisters::Interrupt::MMUCapExceeded))
            dbgln("V3D: Cap exceeded");
        if (has_flag(hub_interrupts, HubRegisters::Interrupt::MMUPageTableInvalid))
            dbgln("V3D: PTE invalid");
        if (has_flag(hub_interrupts, HubRegisters::Interrupt::MMUWriteViolation))
            dbgln("V3D: Write violation");

        auto vaddr = m_hub_registers->mmu_0.fault_vaddr << 4;

        dbgln("V3D: Fault GPU virtual address: {:#08x}", vaddr);
        dbgln("V3D: Fault AXI ID: {:#08x}", m_hub_registers->mmu_0.fault_axi_id);

        // "Cancel" any running jobs.
        m_mmu_faulted.with([](bool& mmu_faulted) { mmu_faulted = true; });
        m_current_binning_job_finished.with([](bool& job_finished) { job_finished = true; });
        m_current_binning_job_finished_wait_queue.notify_one();
        m_current_render_job_finished.with([](bool& job_finished) { job_finished = true; });
        m_current_render_job_finished_wait_queue.notify_one();
    }

    auto core_interrupts = m_core_0_registers->interrupt_status;

    m_core_0_registers->interrupt_clear_pending = core_interrupts;

    if (has_flag(core_interrupts, CoreRegisters::Interrupt::BinningModeFlushDone)) {
        m_current_binning_job_finished.with([](bool& job_finished) { job_finished = true; });
        m_current_binning_job_finished_wait_queue.notify_one();
    }
    if (has_flag(core_interrupts, CoreRegisters::Interrupt::RenderModeFrameDone)) {
        m_current_render_job_finished.with([](bool& job_finished) { job_finished = true; });
        m_current_render_job_finished_wait_queue.notify_one();
    }
    if (has_flag(core_interrupts, CoreRegisters::Interrupt::BinnerOutOfMemory)) {
        dbgln("V3D: Binner out of memory! FIXME: Allocate overspill memory.");
    }
    if (has_flag(core_interrupts, CoreRegisters::Interrupt::BinnerOverspillMemoryInUse)) {
        dbgln("V3D: Binner overspill memory in use");
    }

    return to_underlying(hub_interrupts) != 0 || to_underlying(core_interrupts) != 0;
}

static constinit Array const compatibles_array = {
    "brcm,2712-v3d"sv,
};

DEVICETREE_DRIVER(V3DDriver, compatibles_array);

// https://www.kernel.org/doc/Documentation/devicetree/bindings/gpu/brcm,bcm-v3d.yaml
ErrorOr<void> V3DDriver::probe(DeviceTree::Device const& device, StringView) const
{
    auto hub_registers_resource = TRY(device.get_resource(0));
    auto core_0_registers_resource = TRY(device.get_resource(1));

    auto hub_interrupt_number = TRY(device.get_interrupt_number(0));

    Optional<InterruptNumber> core_interrupt_number;

    auto core_interrupt_number_or_error = device.get_interrupt_number(1);
    if (!core_interrupt_number_or_error.is_error())
        core_interrupt_number = core_interrupt_number_or_error.release_value();

    (void)TRY(V3D::create(hub_registers_resource, core_0_registers_resource, hub_interrupt_number, core_interrupt_number)).leak_ref();

    return {};
}

}
