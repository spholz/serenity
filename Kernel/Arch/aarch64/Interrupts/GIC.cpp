/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/InterruptManagement.h>
#include <Kernel/Arch/aarch64/Interrupts/GIC.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>
#include <Kernel/Time/HardwareTimer.h>

namespace Kernel {

UNMAP_AFTER_INIT ErrorOr<NonnullLockRefPtr<GIC>> GIC::try_to_initialize(DeviceTree::DeviceTreeNodeView const& dt_node)
{
    auto maybe_reg = dt_node.get_property("reg"sv);
    if (!maybe_reg.has_value())
        return EINVAL;

    auto reg = maybe_reg.release_value();

    u32 interrupt_cells = dt_node.get_property("#interrupt-cells"sv).value().as<u32>();
    (void)interrupt_cells;

    auto reg_stream = reg.as_stream();

    auto const* parent = dt_node.parent();
    VERIFY(parent != nullptr);

    auto parent_address_cells = parent->get_property("#address-cells"sv).value().as<u32>();
    auto parent_size_cells = parent->get_property("#size-cells"sv).value().as<u32>();

    PhysicalAddress distributor_registers_paddr;
    size_t distributor_registers_size;
    if (parent_address_cells == 1)
        distributor_registers_paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u32>>()) };
    else if (parent_address_cells == 2)
        distributor_registers_paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u64>>()) };
    else
        return EINVAL;

    if (parent_size_cells == 1)
        distributor_registers_size = MUST(reg_stream.read_value<BigEndian<u32>>());
    else if (parent_address_cells == 2)
        distributor_registers_size = MUST(reg_stream.read_value<BigEndian<u64>>());
    else
        return EINVAL;

    PhysicalAddress cpu_interface_registers_paddr;
    size_t cpu_interface_registers_size;
    if (parent_address_cells == 1)
        cpu_interface_registers_paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u32>>()) };
    else if (parent_address_cells == 2)
        cpu_interface_registers_paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u64>>()) };
    else
        return EINVAL;

    if (parent_size_cells == 1)
        cpu_interface_registers_size = MUST(reg_stream.read_value<BigEndian<u32>>());
    else if (parent_address_cells == 2)
        cpu_interface_registers_size = MUST(reg_stream.read_value<BigEndian<u64>>());
    else
        return EINVAL;

    if (distributor_registers_size < sizeof(DistributorRegisters))
        return EINVAL;

    if (cpu_interface_registers_size < sizeof(CPUInterfaceRegisters))
        return EINVAL;

    // XXX: ranges
    distributor_registers_paddr = distributor_registers_paddr.offset(0xff80'0000 - 0x4000'0000);
    cpu_interface_registers_paddr = cpu_interface_registers_paddr.offset(0xff80'0000 - 0x4000'0000);

    dbgln("GIC distributor registers @ {}", distributor_registers_paddr);
    dbgln("GIC CPU interface registers @ {}", cpu_interface_registers_paddr);

    auto distributor_registers = TRY(Memory::map_typed_writable<DistributorRegisters volatile>(distributor_registers_paddr));
    auto cpu_interface_registers = TRY(Memory::map_typed_writable<CPUInterfaceRegisters volatile>(cpu_interface_registers_paddr));

    auto gic = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) GIC(move(distributor_registers), move(cpu_interface_registers))));
    TRY(gic->initialize());

    return gic;
}

void GIC::enable(GenericInterruptHandler const& handler)
{
    auto interrupt_number = handler.interrupt_number();
    m_distributor_registers->interrupt_set_enable_registers[interrupt_number / 32] = 1 << (interrupt_number % 32);
}

void GIC::disable(GenericInterruptHandler const& handler)
{
    auto interrupt_number = handler.interrupt_number();
    m_distributor_registers->interrupt_clear_enable_registers[interrupt_number / 32] = 1 << (interrupt_number % 32);
}

void GIC::eoi(GenericInterruptHandler const& handler)
{
    auto interrupt_number = handler.interrupt_number();
    m_cpu_interface_registers->end_of_interrupt_register = interrupt_number;
}

Optional<u8> GIC::pending_interrupt() const
{
    auto interrupt_number = m_cpu_interface_registers->interrupt_acknowledge_register;

    // 1023 means no pending interrupt.
    if (interrupt_number == 1023)
        return {};

    return interrupt_number;
}

UNMAP_AFTER_INIT GIC::GIC(Memory::TypedMapping<DistributorRegisters volatile>&& distributor_registers, Memory::TypedMapping<CPUInterfaceRegisters volatile>&& cpu_interface_registers)
    : m_distributor_registers(move(distributor_registers))
    , m_cpu_interface_registers(move(cpu_interface_registers))
{
}

UNMAP_AFTER_INIT ErrorOr<void> GIC::initialize()
{
    if (m_cpu_interface_registers->cpu_interface_identification_register.architecture_version != 2)
        return ENOTSUP; // We only support GICv2 currently

    dbgln("GIC CPU interface architecture version: {:#x}", (u32)m_cpu_interface_registers->cpu_interface_identification_register.architecture_version);
    dbgln("GIC CPU interface implementer: {:#x}", (u32)m_cpu_interface_registers->cpu_interface_identification_register.implementer);
    dbgln("GIC CPU interface product ID: {:#x}", (u32)m_cpu_interface_registers->cpu_interface_identification_register.product_id);

    dbgln("GIC Distributor implementer: {:#x}", (u32)m_distributor_registers->distributor_implementer_identification_register.implementer);
    dbgln("GIC Distributor product ID: {:#x}", (u32)m_distributor_registers->distributor_implementer_identification_register.product_id);
    dbgln("GIC Distributor variant: {:#x}", (u32)m_distributor_registers->distributor_implementer_identification_register.variant);
    dbgln("GIC Distributor revision: {:#x}", (u32)m_distributor_registers->distributor_implementer_identification_register.revision);

    m_distributor_registers->distributor_control_register.enable = 0;

    u32 max_number_of_interrupts = 32 * (m_distributor_registers->interrupt_controller_type_register.it_lines_number + 1);

    for (size_t i = 0; i < max_number_of_interrupts / 32; i++) {
        m_distributor_registers->interrupt_clear_enable_registers[i] = 0xffff'ffff;
        m_distributor_registers->interrupt_clear_pending_registers[i] = 0xffff'ffff;
        m_distributor_registers->clear_active_registers[i] = 0xffff'ffff;
    }

    for (size_t i = 0; i < max_number_of_interrupts / 16; i++) {
        // XXX: Don't touch the reserved field
        m_distributor_registers->interrupt_configuration_registers[i] = 0;
    }

    for (size_t i = 0; i < max_number_of_interrupts / 4; i++) {
        m_distributor_registers->interrupt_priority_registers[i] = static_cast<u32>(explode_byte(0xa0));
        m_distributor_registers->interrupt_processor_targets_registers[i] = static_cast<u32>(explode_byte(0xff));
    }

    m_cpu_interface_registers->interrupt_priority_mask_register.priority = 0xff;
    m_cpu_interface_registers->cpu_interface_control_register.enable = 1;

    m_distributor_registers->distributor_control_register.enable = 1;

    return {};
}

bool HardwareTimer<GenericInterruptHandler>::eoi()
{
    InterruptManagement::the().get_responsible_irq_controller(0)->eoi(*this);
    return true;
}

}
