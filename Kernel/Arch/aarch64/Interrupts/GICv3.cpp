/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/aarch64/InterruptManagement.h>
#include <Kernel/Arch/aarch64/Interrupts/GICv3.h>
#include <Kernel/Firmware/DeviceTree/DeviceTree.h>
#include <Kernel/Firmware/DeviceTree/Driver.h>
#include <Kernel/Firmware/DeviceTree/Management.h>
#include <Kernel/Interrupts/GenericInterruptHandler.h>

namespace Kernel {

// 12.8 The GIC Distributor register map
struct GICv3::DistributorRegisters {
    enum class ControlBits : u32 {
        // EnableGrp0 if the GIC only supports a single Security state
        EnableGroup1 = 1 << 0,

        // EnableGrp1 if the GIC only supports a single Security state
        EnableGroup1A = 1 << 1,
    };

    static constexpr size_t INTERRUPT_CONTROLLER_TYPE_IT_LINES_NUMBER_OFFSET = 0;
    static constexpr u32 INTERRUPT_CONTROLLER_TYPE_IT_LINES_NUMBER_MASK = (1 << 5) - 1;

    u32 control;                     // GICD_CTLR
    u32 interrupt_controller_type;   // GICD_TYPER
    u32 implementer_identification;  // GICD_IIDR
    u32 interrupt_controller_type_2; // GICD_TYPER2
    u32 error_reporting_status;      // GICD_STATUSR, optional
    u32 reserved0[3];
    u32 implementation_defined0[8];
    u32 set_spi_non_secure;
    u32 reserved1;
    u32 clear_spi_non_secure;
    u32 reserved2;
    u32 set_spi_secure;
    u32 reserved3;
    u32 clear_spi_secure;
    u32 reserved4[9];
    u32 interrupt_group[32];         // GICD_IGROUPn
    u32 interrupt_set_enable[32];    // GICD_ISENABLERn
    u32 interrupt_clear_enable[32];  // GICD_ICENABLERn
    u32 interrupt_set_pending[32];   // GICD_ISPENDRn
    u32 interrupt_clear_pending[32]; // GICD_ICPENDRn
    u32 interrupt_set_active[32];    // GICD_ISACTIVERn
    u32 interrupt_clear_active[32];  // GICD_ICACTIVERn
    u32 interrupt_priority[255];     // GICD_IPRIORITYRn
    u32 reserved5;
    u32 interrupt_processor_targets[255]; // GICD_ITARGETSRn, legacy
    u32 reserved6;
    u32 interrupt_configuration[64];   // GICD_ICFGRn
    u32 interrupt_group_modifier[64];  // GICD_IGRPMODRn
    u32 non_secure_access_control[64]; // GICD_NSACRn
    u32 software_generated_interrupt;  // GICD_SGIR, legacy
    u32 reserved8[3];
    u32 software_generated_interrupt_clear_pending[4]; // GICD_CPENDSGIRn
    u32 software_generated_interrupt_set_pending[4];   // GICD_SPENDSGIRn
    u32 reserved9[40];
    // TODO
    // u32 non_maskable_interrupt[32]; // GICD_INMRIRn
    u32 implementation_defined1[12];
};
static_assert(AssertSize<GICv3::DistributorRegisters, 0x1000>());
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, error_reporting_status) == 0x10);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, set_spi_non_secure) == 0x40);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, clear_spi_secure) == 0x58);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, interrupt_priority) == 0x400);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, interrupt_processor_targets) == 0x800);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, non_secure_access_control) == 0xe00);
static_assert(__builtin_offsetof(GICv3::DistributorRegisters, software_generated_interrupt_set_pending) == 0xf20);

AK_ENUM_BITWISE_OPERATORS(GICv3::DistributorRegisters::ControlBits);

// Table 12-27 GIC physical LPI Redistributor register map
struct PhysicalLPIRedistributorRegisters {
    enum class WakeBits : u32 {
        ProcessorSleep = 1 << 1,
        ChildrenAsleep = 1 << 2,
    };

    u32 control;                // GICR_CTLR
    u32 identification;         // GICR_IIDR
    u32 type;                   // GICR_TYPER
    u32 error_reporting_status; // GICR_STATUSR
    u32 wake;                   // GICR_WAKER
    u32 maximum_partid_and_pmg; // GICR_MPAMIDR
    u32 partid;                 // GICR_PARTIDR
    u32 reserved0[9];
    u32 set_lpi_pending;   // GICR_SETLPIR
    u32 clear_lpi_pending; // GICR_CLRLPRIR
    u8 padding[0x1'0000 - 72];
};
static_assert(__builtin_offsetof(PhysicalLPIRedistributorRegisters, set_lpi_pending) == 0x40);
static_assert(AssertSize<PhysicalLPIRedistributorRegisters, 0x1'0000>());

// Table 12-29 GIC SGI and PPI Redistributor register map
struct SGIAndPPIRedistributorRegisters {
    u32 reserved0[32];
    u32 interrupt_group[3]; // GICR_IGROUPR0, GICR_IGROUPRnE
    u32 reserved1[29];
    u32 interrupt_set_enable[3]; // GICR_ISENABLER0, GICR_ISENABLERnE
    u32 reserved2[29];
    u32 interrupt_clear_enable[3]; // GICR_ICENABLER0, GICR_ICENABLERnE
    u32 reserved3[29];
    u32 interrupt_set_pending[3]; // GICR_ISPENDR0, GICR_ISPENDRnE
    u32 reserved4[29];
    u32 interrupt_clear_pending[3]; // GICR_ICPENDR0, GICR_ICPENDRnE
    u32 reserved5[29];
    u32 interrupt_set_active[3]; // GICR_ISACTIVER0, GICR_ISACTIVERnE
    u32 reserved6[29];
    u32 interrupt_clear_active[3]; // GICR_ICACTIVER0, GICR_ICACTIVERnE
    u32 reserved7[29];
    u32 interrupt_priority[3]; // GICR_IPRIORITYR0, GICR_IPRIORITYRnE
    u32 reserved8[29];
    u32 interrupt_configuration[3]; // GICR_ICFGR0, GICR_ICFGRnE
    u32 reserved9[29];
    u8 padding[0x1'0000 - 1280];
};
// static_assert(AssertSize<SGIAndPPIRedistributorRegisters, 0x1'0000>());

// 12.10 The Redistributor register map
struct GICv3::RedistributorRegisters {
    PhysicalLPIRedistributorRegisters physical_lpis_and_overall_behavior;
    SGIAndPPIRedistributorRegisters sgis_and_ppis;
};
static_assert(AssertSize<GICv3::RedistributorRegisters, 2uz * 64 * KiB>());

UNMAP_AFTER_INIT ErrorOr<NonnullLockRefPtr<GICv3>> GICv3::try_to_initialize(DeviceTree::Device::Resource distributor_registers_resource, DeviceTree::Device::Resource redistributor_registers_resource)
{
    if (distributor_registers_resource.size < sizeof(DistributorRegisters))
        return EINVAL;

    if (redistributor_registers_resource.size < sizeof(RedistributorRegisters))
        return EINVAL;

    auto distributor_registers = TRY(Memory::map_typed_writable<DistributorRegisters volatile>(distributor_registers_resource.paddr));
    auto redistributor_registers = TRY(Memory::map_typed_writable<RedistributorRegisters volatile>(redistributor_registers_resource.paddr));

    auto gic = TRY(adopt_nonnull_lock_ref_or_enomem(new (nothrow) GICv3(move(distributor_registers), move(redistributor_registers))));
    TRY(gic->initialize());

    return gic;
}

void GICv3::enable(GenericInterruptHandler const& handler)
{
    // FIXME: Set the trigger mode in DistributorRegisters::interrupt_configuration (GICD_ICFGRn) to level-triggered or edge-triggered.

    auto interrupt_number = handler.interrupt_number();

    if (interrupt_number < 32)
        m_redistributor_registers->sgis_and_ppis.interrupt_set_enable[interrupt_number / 32] = 1 << (interrupt_number % 32);
    else
        m_distributor_registers->interrupt_set_enable[interrupt_number / 32] = 1 << (interrupt_number % 32);
}

void GICv3::disable(GenericInterruptHandler const& handler)
{
    auto interrupt_number = handler.interrupt_number();

    if (interrupt_number < 32)
        m_redistributor_registers->sgis_and_ppis.interrupt_clear_enable[interrupt_number / 32] = 1 << (interrupt_number % 32);
    else
        m_distributor_registers->interrupt_clear_enable[interrupt_number / 32] = 1 << (interrupt_number % 32);
}

void GICv3::eoi(GenericInterruptHandler const& handler)
{
    auto interrupt_number = handler.interrupt_number();
    Aarch64::ICC_EOIR1_EL1::write({ .INTID = interrupt_number });
}

Optional<size_t> GICv3::pending_interrupt() const
{
    auto interrupt_number = Aarch64::ICC_IAR1_EL1::read().INTID;

    // 1023 means no pending interrupt.
    if (interrupt_number == 1023)
        return {};

    return interrupt_number;
}

UNMAP_AFTER_INIT GICv3::GICv3(Memory::TypedMapping<DistributorRegisters volatile> distributor_registers, Memory::TypedMapping<RedistributorRegisters volatile> cpu_interface_registers)
    : m_distributor_registers(move(distributor_registers))
    , m_redistributor_registers(move(cpu_interface_registers))
{
}

UNMAP_AFTER_INIT ErrorOr<void> GICv3::initialize()
{
    // Disable forwarding of interrupts to the redistributors during initialization.
    m_distributor_registers->control &= ~to_underlying(DistributorRegisters::ControlBits::EnableGroup1 | DistributorRegisters::ControlBits::EnableGroup1A);

    auto it_lines_number = (m_distributor_registers->interrupt_controller_type >> DistributorRegisters::INTERRUPT_CONTROLLER_TYPE_IT_LINES_NUMBER_OFFSET) & DistributorRegisters::INTERRUPT_CONTROLLER_TYPE_IT_LINES_NUMBER_MASK;

    // 12.9.38 GICD_TYPER, Interrupt Controller Type Register:
    // "If the value of this field is N, the maximum SPI INTID is 32(N+1) minus 1."
    // "The ITLinesNumber field only indicates the maximum number of SPIs that the GIC implementation might support. This value determines the number of instances of the following interrupt registers [...]"
    u32 const max_number_of_interrupts_excluding_lpis = 32 * (it_lines_number + 1);

    // Disable all interrupts, mark them as non-pending, and assign them to group 1.
    for (size_t i = 0; i < max_number_of_interrupts_excluding_lpis / 32; i++) {
        m_distributor_registers->interrupt_set_enable[i] = 0xffff'ffff;
        m_distributor_registers->interrupt_clear_pending[i] = 0xffff'ffff;
        m_distributor_registers->interrupt_clear_active[i] = 0xffff'ffff;
        m_distributor_registers->interrupt_group[i] = 0xffff'ffff;
    }

    m_redistributor_registers->sgis_and_ppis.interrupt_set_enable[0] = 0xffff'ffff;
    m_redistributor_registers->sgis_and_ppis.interrupt_clear_pending[0] = 0xffff'ffff;
    m_redistributor_registers->sgis_and_ppis.interrupt_clear_active[0] = 0xffff'ffff;
    m_redistributor_registers->sgis_and_ppis.interrupt_group[0] = 0xffff'ffff;

    // Initialize the priority of all interrupts to 0 (the highest priority) and configure them to target all processors.
    for (size_t i = 0; i < max_number_of_interrupts_excluding_lpis / 4; i++) {
        m_distributor_registers->interrupt_priority[i] = 0;
        m_distributor_registers->interrupt_processor_targets[i] = static_cast<u32>(explode_byte(0xff));
    }

    // Enable the distributor.
    m_distributor_registers->control |= to_underlying(DistributorRegisters::ControlBits::EnableGroup1 | DistributorRegisters::ControlBits::EnableGroup1A);

    // Learn the architecture - Generic Interrupt Controller v3 and v4, Overview: 5. Conﬁguring the Arm GIC

    // FIXME: We need to configure the redistributor and CPU interface for each processor once we support SMP.

    // Tell the redistributor that this processor is online.
    m_redistributor_registers->physical_lpis_and_overall_behavior.wake &= ~to_underlying(PhysicalLPIRedistributorRegisters::WakeBits::ProcessorSleep);
    while ((m_redistributor_registers->physical_lpis_and_overall_behavior.wake & to_underlying(PhysicalLPIRedistributorRegisters::WakeBits::ChildrenAsleep)) != 0)
        Processor::pause();

    asm volatile("isb" ::: "memory");

    // Enable the System register interface.
    Aarch64::ICC_SRE_EL1::write({
        .SRE = 1,
        .DFB = 0,
        .DIB = 0,
    });
    asm volatile("isb" ::: "memory");

    // Set the interrupt priority threshold to the max value, so accept any interrupt with a priority below 0xff.
    Aarch64::ICC_PMR_EL1::write({ .Priority = 0xff });
    asm volatile("isb" ::: "memory");

    Aarch64::ICC_BPR1_EL1::write({ .BinaryPoint = 0 });
    asm volatile("isb" ::: "memory");

    Aarch64::ICC_CTLR_EL1::write({
        .CBPR = 0,
        .EOImode = 0,
        .PMHE = 0,
        .PRIbits = 0,
        .IDbits = 0,
        .SEIS = 0,
        .A3V = 0,
        .RSS = 0,
        .ExtRange = 0,
    });

    // Enable Group 1 interrupts.
    Aarch64::ICC_IGRPEN1_EL1::write({ .Enable = 1 });
    asm volatile("isb" ::: "memory");

    return {};
}

static constinit Array const compatibles_array = {
    "arm,gic-v3"sv,
};

EARLY_DEVICETREE_DRIVER(GICv3Driver, compatibles_array);

// https://www.kernel.org/doc/Documentation/devicetree/bindings/interrupt-controller/arm,gic-v3.yaml
ErrorOr<void> GICv3Driver::probe(DeviceTree::Device const& device, StringView) const
{
    // if (!Processor::current().has_feature(CPUFeature::GICv3))
    //     return ENOTSUP;

    auto distributor_registers_resource = TRY(device.get_resource(0));
    auto redistributor_registers_resource = TRY(device.get_resource(1));

    DeviceTree::DeviceRecipe<NonnullLockRefPtr<IRQController>> recipe {
        name(),
        device.node_name(),
        [distributor_registers_resource, redistributor_registers_resource] {
            return GICv3::try_to_initialize(distributor_registers_resource, redistributor_registers_resource);
        },
    };

    InterruptManagement::add_recipe(move(recipe));

    return {};
}
}
