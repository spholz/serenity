/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
 * Copyright (c) 2022, Timon Kruiper <timonkruiper@gmail.com>
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/InterruptManagement.h>
#include <Kernel/Arch/aarch64/Interrupts/GIC.h>
#include <Kernel/Arch/aarch64/RPi/InterruptController.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>

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
#ifdef AARCH64_MACHINE_VIRT
    // XXX: HACK
    auto const& soc_node = DeviceTree::get();
#else
    auto const maybe_soc_node = DeviceTree::get().get_child("soc"sv);
    if (!maybe_soc_node.has_value()) {
        dmesgln("Interrupts: No `soc` node found in the device tree, Interrupts initialization will be skipped");
        return;
    }

    auto const& soc_node = maybe_soc_node.value();
#endif

    enum class ControllerCompatible {
        Unknown,
        BRCM_BCM2836_ARMCTRL_IC, // brcm,bcm2836-armctrl-ic
        ARM_GIC_400,             // arm,gic-400
        ARM_ARM_Cortex_A15_GIC,  // arm,cortex-a15-gic
    };

    auto interrupt_controllers_seen = 0;

    for (auto const& [node_name, node] : soc_node.children()) {
        if (!node.has_property("interrupt-controller"sv))
            continue;

        interrupt_controllers_seen++;

        auto maybe_compatible = node.get_property("compatible"sv);
        if (!maybe_compatible.has_value()) {
            dmesgln("Interrupts: Devicetree node for {} does not have a 'compatible' string, rejecting", node_name);
            continue;
        }
        auto compatible = maybe_compatible.release_value();

        auto controller_compatibility = ControllerCompatible::Unknown;
        compatible.for_each_string([&controller_compatibility](StringView compatible_string) -> IterationDecision {
            if (compatible_string == "brcm,bcm2836-armctrl-ic"sv) {
                controller_compatibility = ControllerCompatible::BRCM_BCM2836_ARMCTRL_IC;
                return IterationDecision::Break;
            }
            if (compatible_string == "arm,gic-400"sv) {
                controller_compatibility = ControllerCompatible::ARM_GIC_400;
                return IterationDecision::Break;
            }
            if (compatible_string == "arm,cortex-a15-gic"sv) {
                controller_compatibility = ControllerCompatible::ARM_ARM_Cortex_A15_GIC;
                return IterationDecision::Break;
            }

            return IterationDecision::Continue;
        });

        if (controller_compatibility == ControllerCompatible::Unknown)
            continue;

        if (!m_interrupt_controllers.is_empty()) {
            dbgln("Ignoring interrupt controller {}. Only one interrupt controller is currently supported.", node_name);
            continue;
        }

        switch (controller_compatibility) {
        case ControllerCompatible::BRCM_BCM2836_ARMCTRL_IC:
            m_interrupt_controllers.append(adopt_lock_ref(*new (nothrow) RPi::InterruptController));
            break;

        case ControllerCompatible::ARM_GIC_400:
        case ControllerCompatible::ARM_ARM_Cortex_A15_GIC: {
            auto maybe_gic = GIC::try_to_initialize(node);
            if (maybe_gic.is_error()) {
                dmesgln("Interrupts: Failed to initialize GIC {}, due to {}", node_name, maybe_gic.release_error());
                continue;
            }

            m_interrupt_controllers.append(maybe_gic.release_value());

            break;
        }

        case ControllerCompatible::Unknown:
            VERIFY_NOT_REACHED();
        }
    }

    if (interrupt_controllers_seen > 0 && m_interrupt_controllers.is_empty())
        dmesgln("Interrupts: {} interrupt controllers seen, but none are compatible", interrupt_controllers_seen);
}

u8 InterruptManagement::acquire_mapped_interrupt_number(u8 interrupt_number)
{
    return interrupt_number;
}

Vector<NonnullLockRefPtr<IRQController>> const& InterruptManagement::controllers()
{
    return m_interrupt_controllers;
}

NonnullLockRefPtr<IRQController> InterruptManagement::get_responsible_irq_controller(u8)
{
    // TODO: Support more interrupt controllers
    VERIFY(m_interrupt_controllers.size() == 1);
    return m_interrupt_controllers[0];
}

void InterruptManagement::enumerate_interrupt_handlers(Function<void(GenericInterruptHandler&)>)
{
    TODO_AARCH64();
}

}
