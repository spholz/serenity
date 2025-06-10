/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Delay.h>
#include <Kernel/Arch/aarch64/ASM_wrapper.h>
#include <Kernel/Arch/aarch64/RPi/V3D/ControlList.h>
#include <Kernel/Arch/aarch64/RPi/V3D/ControlRecords.h>
#include <Kernel/Arch/aarch64/RPi/V3D/Registers.h>
#include <Kernel/Arch/aarch64/RPi/V3D/V3D.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>
#include <Kernel/Memory/MemoryManager.h>

#include "ClearColor.h"
#include "Job.h"
#include "Triangle.h"

namespace Kernel::RPi::V3D {

static void dump_hub_registers(HubRegisters const volatile& registers)
{
    dbgln("V3D Core Registers:");
    dbgln("  UIFCFG: {:#08x}", (u32)registers.uifcfg);
    dbgln("  IDENT0: {:#08x}", (u32)registers.identification_0);
    dbgln("  IDENT1: {:#08x}", (u32)registers.identification_1);
    dbgln("  IDENT2: {:#08x}", (u32)registers.identification_2);
    dbgln("  IDENT3: {:#08x}", (u32)registers.identification_3);
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
    dbgln("    CT0QMA: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_alloc_memory_address);
    dbgln("    CT0QMS: {:#08x}", (u32)registers.control_list_executor.thread_0_tile_alloc_memory_size);
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

ErrorOr<NonnullRefPtr<V3D>> V3D::create(DeviceTree::Device::Resource hub_registers_resource, DeviceTree::Device::Resource core_0_registers_resource)
{
    if (hub_registers_resource.size < sizeof(HubRegisters))
        return EINVAL;

    if (core_0_registers_resource.size < sizeof(CoreRegisters))
        return EINVAL;

    auto hub_registers = TRY(Memory::map_typed_writable<HubRegisters volatile>(hub_registers_resource.paddr));
    auto core_0_registers = TRY(Memory::map_typed_writable<CoreRegisters volatile>(core_0_registers_resource.paddr));

    auto v3d = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) V3D(move(hub_registers), move(core_0_registers))));
    TRY(v3d->initialize());

    return v3d;
}

V3D::V3D(Memory::TypedMapping<HubRegisters volatile> hub_registers, Memory::TypedMapping<CoreRegisters volatile> core_0_registers)
    : m_hub_registers(move(hub_registers))
    , m_core_0_registers(move(core_0_registers))
{
}

ErrorOr<void> V3D::initialize()
{
    dump_hub_registers(*m_hub_registers);
    dump_core_registers(*m_core_0_registers);

    // auto job = run_clear_color(g_boot_info.boot_framebuffer.paddr.get(), 640, 480, g_boot_info.boot_framebuffer.pitch);
    auto job = run_triangle(g_boot_info.boot_framebuffer.paddr.get(), 640, 480, g_boot_info.boot_framebuffer.pitch);

    asm volatile("isb; dsb sy; isb" ::: "memory");

    m_core_0_registers->control_list_executor.thread_0_tile_alloc_memory_address = job.tile_alloc_memory_bo.offset;
    m_core_0_registers->control_list_executor.thread_0_tile_alloc_memory_size = job.tile_alloc_memory_bo.size;

    m_core_0_registers->control_list_executor.thread_0_tile_state_data_array_address = job.tile_state_data_array_bo.offset;

    m_core_0_registers->control_list_executor.thread_0_control_list_start_address = job.binner_control_list.bo().offset;
    m_core_0_registers->control_list_executor.thread_0_control_list_end_address = job.binner_control_list.bo().offset + job.binner_control_list.data().size();

    dump_core_registers(*m_core_0_registers);

    microseconds_delay(10'000);

    dump_core_registers(*m_core_0_registers);

    asm volatile("isb; dsb sy; isb" ::: "memory");

    m_core_0_registers->control_list_executor.thread_1_control_list_start_address = job.render_control_list.bo().offset;
    m_core_0_registers->control_list_executor.thread_1_control_list_end_address = job.render_control_list.bo().offset + job.render_control_list.data().size();

    dump_core_registers(*m_core_0_registers);

    microseconds_delay(10'000);

    dump_core_registers(*m_core_0_registers);

    Processor::halt();

    return {};
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

    (void)TRY(V3D::create(hub_registers_resource, core_0_registers_resource)).leak_ref();

    return {};
}

}
