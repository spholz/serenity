/*
 * Copyright (c) 2024, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Arch/aarch64/IRQController.h>
#include <Kernel/Memory/TypedMapping.h>
#include <LibDeviceTree/DeviceTree.h>

// Only GICv2 is currently supported.
// GICv2 specification: https://documentation-service.arm.com/static/5f8ff21df86e16515cdbfafe
// GIC-400 TRM: https://documentation-service.arm.com/static/5e8f15e27100066a414f7424

namespace Kernel {

class GIC final : public IRQController {
public:
    static ErrorOr<NonnullLockRefPtr<GIC>> try_to_initialize(DeviceTree::DeviceTreeNodeView const&);

    void enable(GenericInterruptHandler const&) override;
    void disable(GenericInterruptHandler const&) override;

    void eoi(GenericInterruptHandler const&) override;

    Optional<u8> pending_interrupt() const override;

    StringView model() const override { return "GIC"sv; }

private:
    struct DistributorRegisters {
        struct {
            u32 enable : 1;
            u32 : 31;
        } distributor_control_register; // GICD_CTLR
        struct {
            u32 it_lines_number : 5;
            u32 cpu_number : 3;
            u32 : 2;
            u32 security_extn : 1;
            u32 lspi : 5;
            u32 : 16;
        } interrupt_controller_type_register; // GICR_TYPER
        struct {
            u32 implementer : 12;
            u32 revision : 4;
            u32 variant : 4;
            u32 : 4;
            u32 product_id : 8;
        } distributor_implementer_identification_register; // GICD_IIDR
        u32 reserved0[5];
        u32 implementation_defined0[8];
        u32 reserved1[16];
        u32 interrupt_group_registers[32];         // GICD_IGROUPn
        u32 interrupt_set_enable_registers[32];    // GICD_ISENABLERn
        u32 interrupt_clear_enable_registers[32];  // GICD_ICENABLERn
        u32 interrupt_set_pending_registers[32];   // GICD_ISPENDRn
        u32 interrupt_clear_pending_registers[32]; // GICD_ICPENDRn
        u32 set_active_registers[32];              // GICD_ISACTIVERn
        u32 clear_active_registers[32];            // GICD_ICACTIVERn
        u32 interrupt_priority_registers[255];     // GICD_IPRIORITYRn
        u32 reserved2;
        u32 interrupt_processor_targets_registers[255]; // GICD_ITARGETSRn
        u32 reserved3;
        u32 interrupt_configuration_registers[64]; // GICD_ICFGRn
        u32 reserved4[64];
        u32 non_secure_access_control_registers[64]; // GICD_NSACRn
        u32 software_generated_interrupt_register;   // GICD_SGIR
        u32 reserved5[3];
        u32 software_generated_interrupt_clear_pending_registers[4]; // GICD_CPENDSGIRn
        u32 software_generated_interrupt_set_pending_registers[4];   // GICD_SPENDSGIRn
        u32 reserved6[40];
        u32 implementation_defined1[12];
    };
    static_assert(AssertSize<DistributorRegisters, 0x1000>());
    static_assert(__builtin_offsetof(DistributorRegisters, reserved0) == 0x0c);
    static_assert(__builtin_offsetof(DistributorRegisters, implementation_defined0) == 0x20);
    static_assert(__builtin_offsetof(DistributorRegisters, interrupt_group_registers) == 0x80);
    static_assert(__builtin_offsetof(DistributorRegisters, interrupt_processor_targets_registers) == 0x800);
    static_assert(__builtin_offsetof(DistributorRegisters, interrupt_configuration_registers) == 0xc00);
    static_assert(__builtin_offsetof(DistributorRegisters, non_secure_access_control_registers) == 0xe00);
    static_assert(__builtin_offsetof(DistributorRegisters, software_generated_interrupt_clear_pending_registers) == 0xf10);
    static_assert(__builtin_offsetof(DistributorRegisters, software_generated_interrupt_set_pending_registers) == 0xf20);
    static_assert(__builtin_offsetof(DistributorRegisters, implementation_defined1) == 0xfd0);

    struct CPUInterfaceRegisters {
        struct {
            // XXX: Security extension?
            u32 enable : 1;
            u32 : 31;
        } cpu_interface_control_register; // GICC_CTLR
        struct {
            u32 priority : 8;
            u32 : 24;
        } interrupt_priority_mask_register; // GICC_PMR
        u32 binary_point_register;          // GICC_BPR
        u32 interrupt_acknowledge_register; // GICC_IAR
        u32 end_of_interrupt_register;      // GICC_EOIR
        struct {
            u32 priority : 8;
            u32 : 24;
        } running_priority_register;                             // GICC_RPR
        u32 highest_priority_pending_interrupt_register;         // GICC_HPPIR
        u32 aliased_binary_point_register;                       // GICC_ABPR
        u32 aliased_interrupt_acknowledge_register;              // GICC_AIAR
        u32 aliased_end_of_interrupt_register;                   // GICC_AEOIR
        u32 aliased_highest_priority_pending_interrupt_register; // GICC_AHPPIR
        u32 reserved0[5];
        u32 implementation_defined0[36];
        u32 active_priorities_registers[4];            // GICC_APRn
        u32 non_secure_active_priorities_registers[4]; // GICC_NSAPRn
        u32 reserved1[3];
        struct {
            u32 implementer : 12;
            u32 revision : 4;
            u32 architecture_version : 4;
            u32 product_id : 12;
        } cpu_interface_identification_register; // GICC_IIDR
        u32 reserved2[960];
        u32 deactivate_interrupt_register; // GICC_DIR
    };
    static_assert(AssertSize<CPUInterfaceRegisters, 0x1004>());
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, aliased_highest_priority_pending_interrupt_register) == 0x28);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, reserved0) == 0x2c);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, implementation_defined0) == 0x40);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, active_priorities_registers) == 0xd0);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, non_secure_active_priorities_registers) == 0xe0);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, cpu_interface_identification_register) == 0xfc);
    static_assert(__builtin_offsetof(CPUInterfaceRegisters, deactivate_interrupt_register) == 0x1000);

    GIC(Memory::TypedMapping<DistributorRegisters volatile>&&, Memory::TypedMapping<CPUInterfaceRegisters volatile>&&);

    ErrorOr<void> initialize();

    Memory::TypedMapping<DistributorRegisters volatile> m_distributor_registers;
    Memory::TypedMapping<CPUInterfaceRegisters volatile> m_cpu_interface_registers;
};

}
