/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Firmware/DeviceTree/Device.h>
#include <Kernel/Interrupts/IRQHandler.h>
#include <Kernel/Memory/TypedMapping.h>
#include <Kernel/Tasks/WaitQueue.h>

struct V3DJob;

namespace Kernel::RPi::V3D {

struct HubRegisters;
struct CoreRegisters;
class GPU3DDevice;

struct PageTable {
    NonnullOwnPtr<Memory::Region> region;
};

class V3D final : public AtomicRefCounted<V3D> {
public:
    static ErrorOr<NonnullRefPtr<V3D>> create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource, InterruptNumber hub_interrupt_number, Optional<InterruptNumber> core_interrupt_number);
    ErrorOr<PageTable> allocate_page_table();
    // void free_page_table(PageTable&&);

    // FIXME: GPUVirtualAddress type?
    void insert_page_table_entries_for_buffer(PageTable&, u32 gpu_vaddr, Memory::VMObject const&);
    void remove_page_table_entries_for_buffer(PageTable&, u32 gpu_vaddr, Memory::VMObject const&);

    struct AddressRange {
        Memory::VMObject& vmobject;
        FlatPtr gpu_vaddr;
    };
    ErrorOr<void> submit_job(PageTable const&, V3DJob const&);

private:
    V3D(Memory::TypedMapping<HubRegisters volatile>, Memory::TypedMapping<CoreRegisters volatile>, InterruptNumber hub_interrupt_number, Optional<InterruptNumber> core_interrupt_number);

    ErrorOr<void> initialize();

    void flush_mmuc_and_tlb();
    void activate_page_table(PageTable const&);

    bool handle_interrupt();

    Memory::TypedMapping<HubRegisters volatile> m_hub_registers;
    Memory::TypedMapping<CoreRegisters volatile> m_core_0_registers;

    RefPtr<GPU3DDevice> m_3d_device;

    NonnullRefPtr<Memory::PhysicalRAMPage> m_illegal_vaddr_target_page;

    class InterruptHandler : public IRQHandler {
    public:
        InterruptHandler(V3D& v3d, InterruptNumber interrupt_number)
            : IRQHandler(interrupt_number)
            , m_v3d(v3d)
        {
            enable_irq();
        }

        virtual bool handle_irq() override
        {
            return m_v3d.handle_interrupt();
        }

    private:
        V3D& m_v3d;
    };

    InterruptHandler m_hub_interrupt_handler;
    Optional<InterruptHandler> m_core_interrupt_handler;

    SpinlockProtected<bool, LockRank::None> m_mmu_faulted { false };

    SpinlockProtected<bool, LockRank::None> m_current_binning_job_finished { false };
    WaitQueue m_current_binning_job_finished_wait_queue;

    SpinlockProtected<bool, LockRank::None> m_current_render_job_finished { false };
    WaitQueue m_current_render_job_finished_wait_queue;
};

}
