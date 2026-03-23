/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Arch/MemoryFences.h>
#include <Kernel/Bus/PCI/BarMapping.h>
#include <Kernel/Devices/Storage/NVMe/PCIeController.h>
#include <Kernel/Devices/Storage/StorageManagement.h>

namespace Kernel::NVMe {

ErrorOr<NonnullRefPtr<PCIeController>> PCIeController::create(PCI::DeviceIdentifier const& pci_device_identifier, bool nvme_poll)
{
    (void)nvme_poll; // <- XXX

    PCI::enable_memory_space(pci_device_identifier);
    PCI::enable_bus_mastering(pci_device_identifier);

    auto registers = TRY(PCI::map_bar<u8 volatile>(pci_device_identifier, PCI::HeaderType0BaseRegister::BAR0));

    auto controller = TRY(adopt_nonnull_ref_or_enomem(new (nothrow) PCIeController(pci_device_identifier, move(registers))));

    TRY(controller->initialize());

    return controller;
}

LockRefPtr<StorageDevice> PCIeController::device(u32 index) const
{
    return m_storage_devices[index];
}

size_t PCIeController::devices_count() const
{
    return m_storage_devices.size();
}

void PCIeController::complete_current_request(AsyncDeviceRequest::RequestResult)
{
    VERIFY_NOT_REACHED();
}

PCIeController::PCIeController(PCI::DeviceIdentifier const& pci_device_identifier, Memory::TypedMapping<u8 volatile> registers)
    : StorageController(StorageManagement::generate_relative_nvme_controller_id({}))
    , PCI::Device(pci_device_identifier)
    , m_registers(move(registers))
{
}

void PCIeController::set_property32(PropertyOffset property_offset, u32 value)
{
    *reinterpret_cast<u32 volatile*>(m_registers.ptr() + to_underlying(property_offset)) = value;
}

void PCIeController::set_property64(PropertyOffset property_offset, u64 value)
{
    *reinterpret_cast<u64 volatile*>(m_registers.ptr() + to_underlying(property_offset)) = value;
}

u32 PCIeController::get_property32(PropertyOffset property_offset)
{
    return *reinterpret_cast<u32 volatile*>(m_registers.ptr() + to_underlying(property_offset));
}

u64 PCIeController::get_property64(PropertyOffset property_offset)
{
    return *reinterpret_cast<u64 volatile*>(m_registers.ptr() + to_underlying(property_offset));
}

void PCIeController::ring_submission_queue_tail_doorbell(size_t queue_identifier, u32 new_tail_value)
{
    // Figure 4: PCI Express Specific Controller Property Definitions
    auto doorbell_index = queue_identifier * 2;
    auto doorbell_offset = 0x1000 + (doorbell_index * m_doorbell_stride);

    *reinterpret_cast<u32 volatile*>(m_registers.ptr() + doorbell_offset) = new_tail_value;
}

void PCIeController::ring_completion_queue_head_doorbell(size_t queue_identifier, u32 new_head_value)
{
    // Figure 4: PCI Express Specific Controller Property Definitions
    auto doorbell_index = (queue_identifier * 2) + 1;
    auto doorbell_offset = 0x1000 + (doorbell_index * m_doorbell_stride);

    *reinterpret_cast<u32 volatile*>(m_registers.ptr() + doorbell_offset) = new_head_value;
}

ErrorOr<void> PCIeController::initialize()
{
    // 3.5.1 Memory-based Controller Initialization (PCIe)

    auto version = get_property<Version>();
    dmesgln("NVMe: Supported specification version: {}.{}.{}", version.major_version, version.minor_version, version.tertiary_version);

    auto controller_capabilities = get_property<ControllerCapabilities>();

    if (!has_flag(controller_capabilities.command_sets_supported, ControllerCapabilities::CommandSetsSupported::NVMCommandSet)) {
        dmesgln("NVMe: Controller doesn't support the NVM command set");
        return ENOTSUP;
    }

    // "This field indicates the minimum host memory page size that the controller supports. The minimum memory page size is (2 ^ (12 + MPSMIN))."
    auto min_page_size = 1 << (12 + controller_capabilities.memory_page_size_minimum);

    // "This field indicates the maximum host memory page size that the controller supports. The maximum memory page size is (2 ^ (12 + MPSMAX))."
    auto max_page_size = 1 << (12 + controller_capabilities.memory_page_size_maximum);

    auto max_io_queue_entries_supported = controller_capabilities.maximum_queue_entries_supported + 1;
    if (max_io_queue_entries_supported < 2) {
        dmesgln("NVMe: Invalid CAP.MQES value: {}", controller_capabilities.maximum_queue_entries_supported);
        return EINVAL;
    }

    // "This property indicates the stride between doorbell properties. The stride is specified as (2 ^ (2 + DSTRD)) in bytes."
    m_doorbell_stride = 1 << (2 + controller_capabilities.doorbell_stride);

    dbgln("NVMe: Min page size: {:#x}, max page size: {:#x}, max I/O Queue entries supported: {}, doorbell stride: {:#x}", min_page_size, max_page_size, max_io_queue_entries_supported, m_doorbell_stride);

    if (min_page_size > PAGE_SIZE) {
        // XXX: TODO
        return ENOTIMPL;
    }

    // We support this controller, now initialize it!

    // The controller initialization steps seem to assume that the controller is already disabled,
    // but at least some firmwares leave it enabled, therefore disable it first.
    {
        // Ensure the controller is disabled (the firmware might have left it enabled).
        auto controller_configuration = get_property<ControllerConfiguration>();

        controller_configuration.enable = 0;

        set_property<ControllerConfiguration>(controller_configuration);
    }

    // Wait for it to become disabled.
    while (get_property<ControllerStatus>().ready != 0)
        Processor::pause();

    {
        auto controller_configuration = get_property<ControllerConfiguration>();

        // XXX: This field may only be changed when the controler is disabled.
        controller_configuration.io_command_set_selected = ControllerConfiguration::IOCommandSetSelected::NVMCommandSet;

        // "This field indicates the host memory page size. The memory page size is (2 ^ (12 + MPS))."
        controller_configuration.memory_page_size = AK::log2(PAGE_SIZE) - 12;

        // Only round robin is supported by all controllers, see the description of the CAP.AMS field in section 3.1.4.1.
        controller_configuration.arbitration_mechanism_selected = ControllerConfiguration::ArbitrationMechanismSelected::RoundRobin;

        set_property<ControllerConfiguration>(controller_configuration);
    }

    auto admin_submission_queue_dma_buffer = TRY(allocate_contiguous_dma_buffer("NVMe Admin Submission Queue"sv, Memory::Region::Access::Write, PAGE_SIZE));
    m_admin_submission_queue = TRY(try_make<SubmissionQueue>(move(admin_submission_queue_dma_buffer), 2, 64));

    dbgln("NVMe: Admin Submission Queue @ {:#x}", m_admin_submission_queue->dma_base_address());

    auto admin_completion_queue_dma_buffer = TRY(allocate_contiguous_dma_buffer("NVMe Admin Completion Queue"sv, Memory::Region::Access::Read, PAGE_SIZE));
    m_admin_completion_queue = TRY(try_make<CompletionQueue>(move(admin_completion_queue_dma_buffer), 2, 16));

    dbgln("NVMe: Admin Completion Queue @ {:#x}", m_admin_completion_queue->dma_base_address());

    set_property<AdminQueueAttributes>({
        .admin_submission_queue_size = static_cast<u32>(m_admin_submission_queue->size()),
        .admin_completion_queue_size = static_cast<u32>(m_admin_completion_queue->size()),
    });
    set_property<AdminCompletionQueueBaseAddress>({
        .admin_completion_queue_base = m_admin_completion_queue->dma_base_address(),
    });
    set_property<AdminSubmissionQueueBaseAddress>({
        .admin_submission_queue_base = m_admin_submission_queue->dma_base_address(),
    });

    {
        auto controller_configuration = get_property<ControllerConfiguration>();

        controller_configuration.enable = 1;

        set_property<ControllerConfiguration>(controller_configuration);
    }

    while (get_property<ControllerStatus>().ready == 0)
        Processor::pause();

    {
        SubmissionQueueEntry submission {};
        submission.identify_command.opcode = to_underlying(AdminOpcode::Identify);
        submission.identify_command.fused_operation = CommandDword0::FusedOperation::NormalOperation;
        submission.identify_command.prp_or_sgl_for_data_transfer = CommandDword0::PhysicalRegionPageOrScatterGatherListForDataTransfer::PhysicalRegionPageUsed;
        submission.identify_command.command_identifier = 0;

        auto identify_dma_buffer = TRY(allocate_contiguous_dma_buffer("NVMe Identify Command Buffer"sv, Memory::Region::Access::Read, sizeof(IdentifyControllerDataStrucutre)));
        dbgln("NVMe: Identify DMA Buffer @ {:#x}", identify_dma_buffer.bus_address());

        submission.identify_command.data_pointer.physical_region_page.entry_1.physical_region_page_entry.page_base_address_and_offset = identify_dma_buffer.bus_address();
        submission.identify_command.data_pointer.physical_region_page.entry_2.physical_region_page_entry.page_base_address_and_offset = 0;
        submission.identify_command.controller_identifier = 0;
        submission.identify_command.controller_or_namespace_structure = ControllerOrNamespaceStructure::IdentifyControllerDataStructure;
        submission.identify_command.command_set_identifier = CommandSetIdentifier::NVMCommandSet;

        // dbgln("{:hex-dump}", ReadonlyBytes { &submission, sizeof(submission) });

        store_memory_fence();

        TRY(m_admin_submission_queue->submit(submission));

        ring_submission_queue_tail_doorbell(0, m_admin_submission_queue->current_tail_index());

        while (m_admin_completion_queue->is_empty())
            Processor::pause();

        auto completion = TRY(m_admin_completion_queue->dequeue());
        load_memory_fence(); // XXX <- wrong
        dbgln("status: {:#x}", completion.common.status);

        auto const* controller_data_structure = reinterpret_cast<IdentifyControllerDataStrucutre const*>(identify_dma_buffer.virtual_address().as_ptr());

        auto nvme_ascii_string = []<size_t N>(char const(&nvme_string)[N]) {
            // "If padding is necessary, then the string shall be padded with spaces (i.e., ASCII character 20h)
            //  to the right unless the string is specified as null-terminated."

            // XXX: Check that this only contains ASCII chars 0x20-0x7e.
            return StringView { nvme_string, sizeof(nvme_string) }.trim(" \0"sv, TrimMode::Right);
        };

        dbgln("NVMe: Model=\"{}\", Serial=\"{}\", Firmware=\"{}\"",
            nvme_ascii_string(controller_data_structure->model_number),
            nvme_ascii_string(controller_data_structure->serial_number),
            nvme_ascii_string(controller_data_structure->firmware_revision));

        ring_completion_queue_head_doorbell(0, m_admin_completion_queue->current_head_index());
    }

    return {};
}

}
