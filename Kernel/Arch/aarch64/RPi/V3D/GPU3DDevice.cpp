/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/Ioctl.h>
#include <Kernel/Arch/aarch64/RPi/V3D/GPU3DDevice.h>

namespace Kernel::RPi::V3D {

ErrorOr<NonnullRefPtr<GPU3DDevice>> GPU3DDevice::create(V3D& v3d)
{
    return TRY(Device::try_create_device<GPU3DDevice>(v3d));
}

ErrorOr<void> GPU3DDevice::attach(OpenFileDescription& description)
{
    auto page_table = TRY(PageTable::create());
    auto context = TRY(try_make_ref_counted<Context>(description, move(page_table)));

    m_context_list.with([&context](auto& context_list) {
        context_list.append(context);
    });

    return CharacterDevice::attach(description);
}

void GPU3DDevice::detach(OpenFileDescription& description)
{
    auto context = context_for_description(description);

    // XXX: Or should we put this in the Context destructor?
    //      And make it a class then?

    while (!context->buffers.is_empty()) {
        auto const& buffer = context->buffers.first();

        // Use MUST() because the gpu_vaddr should always be valid.
        MUST(free_buffer(context, buffer.gpu_vaddr));
    }

    context->list_node.remove();

    CharacterDevice::detach(description);
}

ErrorOr<void> GPU3DDevice::ioctl(OpenFileDescription& description, unsigned request, Userspace<void*> arg)
{
    auto context = context_for_description(description);

    switch (request) {
    case V3D_ALLOCATE_BUFFER: {
        auto buffer_allocation_info = TRY(copy_typed_from_user(static_ptr_cast<V3DBuffer const*>(arg)));

        auto gpu_vaddr = TRY(allocate_buffer(*context, buffer_allocation_info.size));

        buffer_allocation_info.gpu_virtual_address = gpu_vaddr.value();
        TRY(copy_to_user(static_ptr_cast<V3DBuffer*>(arg), &buffer_allocation_info));

        return {};
    }

    case V3D_FREE_BUFFER: {
        auto buffer_gpu_vaddr = arg.ptr();
        return free_buffer(*context, buffer_gpu_vaddr);
    }

    case V3D_SUBMIT_JOB: {
        auto job = TRY(copy_typed_from_user(static_ptr_cast<V3DJob const*>(arg)));

        auto result = m_v3d->submit_job(context->page_table, job);

        if (result.is_error()) {
            dbgln("SUBMIT_JOB args:");
            dbgln("  tile_state_data_array_base_address={:#08x}", job.tile_state_data_array_base_address);
            dbgln("  tile_allocation_memory_base_address={:#08x}", job.tile_allocation_memory_base_address);
            dbgln("  tile_allocation_memory_size={:#08x}", job.tile_allocation_memory_size);
            dbgln("  binning_control_list_address={:#08x}", job.binning_control_list_address);
            dbgln("  binning_control_list_size={:#08x}", job.binning_control_list_size);
            dbgln("  rendering_control_list_address={:#08x}", job.rendering_control_list_address);
            dbgln("  rendering_control_list_size={:#08x}", job.rendering_control_list_size);

            dbgln("Buffers:");
            for (auto const& buffer : context->buffers)
                dbgln("  GV{:p}-GV{:p}", buffer.gpu_vaddr, buffer.gpu_vaddr.value() + buffer.vmobject->size());
        }

        return result;
    }
    }

    return EINVAL;
}

ErrorOr<File::VMObjectAndMemoryType> GPU3DDevice::vmobject_and_memory_type_for_mmap(OpenFileDescription& description, Memory::VirtualRange const&, u64& offset, bool)
{
    if ((offset % PAGE_SIZE) != 0)
        return EINVAL;

    auto context = context_for_description(description);

    for (auto const& buffer : context->buffers) {
        if (buffer.gpu_vaddr == offset) {
            offset = 0;

            return VMObjectAndMemoryType {
                .vmobject = buffer.vmobject,
                .memory_type = Memory::MemoryType::NonCacheable,
            };
        }
    }

    return EFAULT;
}

GPU3DDevice::GPU3DDevice(V3D& v3d)
    : CharacterDevice(MajorAllocation::CharacterDeviceFamily::GPURender, 0) // XXX: Don't hardcode minor id
    , m_v3d(v3d)
{
}

ErrorOr<GPUVirtualAddress> GPU3DDevice::allocate_buffer(Context& context, size_t size)
{
    // XXX: Check additionally V3D page size.
    if ((size % PAGE_SIZE) != 0)
        return EINVAL;

    // We need to use AllocateNow since we don't want to (and can't even) lazily page in data for the GPU.
    // Page faults don't seem to be recoverable on the V3D.
    auto vmobject = TRY(Memory::AnonymousVMObject::try_create_with_size(size, AllocationStrategy::AllocateNow));

    auto region = TRY(Memory::Region::create_unbacked());
    TRY(context.region_tree.place_anywhere(*region, Memory::RandomizeVirtualAddress::No, size, 4096));

    auto gpu_vaddr = GPUVirtualAddress { static_cast<u32>(region->vaddr().get()) };

    // FIXME: This requires special handling if V3D page size != PAGE_SIZE.

    TRY(context.buffers.try_append({
        .vmobject = move(vmobject),
        .gpu_vaddr = static_cast<u32>(gpu_vaddr.value()),
        .region = move(region),
    }));

    auto const& buffer = context.buffers.last();
    m_v3d->map_buffer(context.page_table, gpu_vaddr.value(), buffer.vmobject);

    return gpu_vaddr;
}

ErrorOr<void> GPU3DDevice::free_buffer(Context& context, GPUVirtualAddress buffer_gpu_vaddr)
{
    // XXX: Should we allow freeing buffers if they are still mmap()ed?
    //      If we allow that, the buffer will stay alive because the Region will keep the refcount of the VMObject nonzero.
    auto buffer_index = context.buffers.find_first_index_if([buffer_gpu_vaddr](Context::Buffer const& buffer) {
        return buffer.gpu_vaddr == buffer_gpu_vaddr;
    });

    if (!buffer_index.has_value())
        return EINVAL;

    auto const& buffer = context.buffers[*buffer_index];

    m_v3d->unmap_buffer(context.page_table, buffer.gpu_vaddr, *buffer.vmobject);

    context.region_tree.remove(*buffer.region);
    context.buffers.remove(*buffer_index);

    return {};
}

}
