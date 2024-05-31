/*
 * Copyright (c) 2023, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/Interrupts.h>
#include <Kernel/Arch/riscv64/IRQController.h>
#include <Kernel/Arch/riscv64/InterruptManagement.h>
#include <Kernel/Arch/riscv64/Interrupts/PLIC.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Interrupts/SharedIRQHandler.h>

namespace Kernel {

static InterruptManagement* s_interrupt_management;

bool InterruptManagement::initialized()
{
    return s_interrupt_management != nullptr;
}

InterruptManagement& InterruptManagement::the()
{
    VERIFY(InterruptManagement::initialized());
    return *s_interrupt_management;
}

void InterruptManagement::initialize()
{
    VERIFY(!InterruptManagement::initialized());
    s_interrupt_management = new InterruptManagement;

    the().find_controllers();
}

void InterruptManagement::find_controllers()
{
    auto const& device_tree = DeviceTree::get();
    auto const maybe_soc_node = DeviceTree::get().get_child("soc"sv);
    if (!maybe_soc_node.has_value()) {
        return;
    }
    auto const& soc_node = maybe_soc_node.value();
    auto soc_address_cells = soc_node.get_property("#address-cells"sv).value().as<u32>();
    auto soc_size_cells = soc_node.get_property("#size-cells"sv).value().as<u32>();

    enum class ControllerCompatible {
        Unknown,
        SiFive_PLIC_1p0p0, // sifive,plic-1.0.0
        RISCV_PLIC0,       // riscv,plic0
    };

    bool plic_found_and_initialized = false;

    for (auto const& [node_name, node] : soc_node.children()) {
        if (!node.has_property("interrupt-controller"sv))
            continue;

        auto maybe_compatible = node.get_property("compatible"sv);
        if (!maybe_compatible.has_value())
            continue;

        auto controller_compatibility = ControllerCompatible::Unknown;
        maybe_compatible.value().for_each_string([&controller_compatibility](StringView compatible_string) -> IterationDecision {
            if (compatible_string == "sifive,plic-1.0.0"sv) {
                controller_compatibility = ControllerCompatible::SiFive_PLIC_1p0p0;
                return IterationDecision::Break;
            }
            if (compatible_string == "riscv,plic0"sv) {
                controller_compatibility = ControllerCompatible::RISCV_PLIC0;
                return IterationDecision::Break;
            }

            return IterationDecision::Continue;
        });

        if (controller_compatibility == ControllerCompatible::Unknown)
            continue;

        if (plic_found_and_initialized) {
            dbgln("InterruptManagement: Ignoring PLIC \"{}\". Only one PLIC is currently supported.", node_name);
            continue;
        }

        u32 interrupt_cells = node.get_property("#interrupt-cells"sv).value().as<u32>();
        u32 riscv_ndev = node.get_property("riscv,ndev"sv).value().as<u32>();
        auto maybe_address_cells = node.get_property("#address-cells"sv);
        if (maybe_address_cells.has_value()) {
            if (maybe_address_cells.value().as<u32>() != 0)
                dbgln("InterruptManagement: PLIC \"{}\" has an invalid #address-cells value: {}", node_name, maybe_address_cells.value().as<u32>());
        } else {
            dbgln("InterruptManagement: PLIC \"{}\" is missing the #address-cells property", node_name);
        }

        (void)interrupt_cells;

        auto reg_stream = node.get_property("reg"sv).value().as_stream();

        PhysicalAddress paddr;
        if (soc_address_cells == 1)
            paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u32>>()) };
        else if (soc_address_cells == 2)
            paddr = PhysicalAddress { MUST(reg_stream.read_value<BigEndian<u64>>()) };
        else
            VERIFY_NOT_REACHED();

        size_t size;
        if (soc_size_cells == 1)
            size = MUST(reg_stream.read_value<BigEndian<u32>>());
        else if (soc_size_cells == 2)
            size = MUST(reg_stream.read_value<BigEndian<u64>>());
        else
            VERIFY_NOT_REACHED();

        // Get the context ID for the supervisor mode context of the boot hart.
        // FIXME: Support multiple contexts when we support SMP on riscv64.
        size_t boot_hart_supervisor_mode_context_id;
        FlatPtr boot_hart_id = s_boot_info.mhartid;

        auto interrupts_extended_stream = node.get_property("interrupts-extended"sv).value().as_stream();

        for (size_t context_id = 0;; context_id++) {
            u32 cpu_intc_phandle = MUST(interrupts_extended_stream.read_value<BigEndian<u32>>());
            auto const* cpu_intc = device_tree.phandle(cpu_intc_phandle);
            VERIFY(cpu_intc != nullptr);

            VERIFY(cpu_intc->has_property("interrupt-controller"sv));
            VERIFY(cpu_intc->get_property("compatible"sv).value().as_strings().contains_slow("riscv,cpu-intc"sv));

            VERIFY(cpu_intc->get_property("#interrupt-cells"sv).value().as<u32>() == 1);

            auto const* cpu = cpu_intc->parent();
            VERIFY(cpu != nullptr);
            VERIFY(cpu->get_property("compatible"sv).value().as_strings().contains_slow("riscv"sv));

            auto const* cpus = cpu->parent();
            VERIFY(cpus != nullptr);
            VERIFY(device_tree.get_child("cpus"sv).value().get_property("#address-cells"sv).value().as<u32>() == 1);
            VERIFY(cpus->get_property("#address-cells"sv).value().as<u32>() == 1);
            VERIFY(cpus->get_property("#size-cells"sv).value().as<u32>() == 0);

            auto cpu_hart_id = cpu->get_property("reg"sv).value().as<u32>();
            u32 interrupt_specifier = MUST(interrupts_extended_stream.read_value<BigEndian<u32>>());
            if (cpu_hart_id == boot_hart_id && interrupt_specifier == (to_underlying(RISCV64::CSR::SCAUSE::SupervisorExternalInterrupt) & ~RISCV64::CSR::SCAUSE_INTERRUPT_MASK)) {
                boot_hart_supervisor_mode_context_id = context_id;
                break;
            }
        }

        // m_interrupt_controllers.append(RISCV64::PLIC::try_to_initialize(paddr, riscv_ndev, boot_hart_supervisor_mode_context_id).release_value_but_fixme_should_propagate_errors());
        m_interrupt_controllers.append(adopt_lock_ref(*new (nothrow) PLIC(PhysicalAddress(paddr), size, riscv_ndev + 1, boot_hart_supervisor_mode_context_id)));
    }
}

u8 InterruptManagement::acquire_mapped_interrupt_number(u8 original_irq)
{
    return original_irq;
}

Vector<NonnullLockRefPtr<IRQController>> const& InterruptManagement::controllers()
{
    return m_interrupt_controllers;
}

NonnullLockRefPtr<IRQController> InterruptManagement::get_responsible_irq_controller(size_t)
{
    // TODO: Support more interrupt controllers
    VERIFY(m_interrupt_controllers.size() == 1);
    return m_interrupt_controllers[0];
}

void InterruptManagement::enumerate_interrupt_handlers(Function<void(GenericInterruptHandler&)> callback)
{
    for (size_t i = 0; i < 64; i++) {
        auto& handler = get_interrupt_handler(i);
        if (handler.type() == HandlerType::SharedIRQHandler) {
            static_cast<SharedIRQHandler&>(handler).enumerate_handlers(callback);
            continue;
        }
        if (handler.type() != HandlerType::UnhandledInterruptHandler)
            callback(handler);
    }
}

}
