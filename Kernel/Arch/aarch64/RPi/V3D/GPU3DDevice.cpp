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
    auto context = make_ref_counted<Context>(description, move(page_table));

    m_context_list.with([&context](auto& context_list) {
        context_list.append(context);
    });

    return CharacterDevice::attach(description);
}

void GPU3DDevice::detach(OpenFileDescription& description)
{
    m_context_list.with([&description, this](auto& list) {
        for (auto& context : list) {
            if (&context.associated_description == &description) {
                // XXX: Or should we put this in the PerContextState destructor?
                //      And make it a class then?

                Vector<u32> buffer_gpu_vaddrs;
                for (auto const& buffer : context.buffers)
                    buffer_gpu_vaddrs.try_append(buffer.gpu_vaddr).release_value_but_fixme_should_propagate_errors();

                for (u32 buffer_gpu_vaddr : buffer_gpu_vaddrs)
                    MUST(free_buffer(context, buffer_gpu_vaddr)); // MUST() because the id should always be valid.

                context.list_node.remove();
                return;
            }
        }

        VERIFY_NOT_REACHED();
    });

    CharacterDevice::detach(description);
}

ErrorOr<void> GPU3DDevice::ioctl(OpenFileDescription& description, unsigned request, Userspace<void*> arg)
{
    auto with_context_for_description = [this](OpenFileDescription& description, auto&& callback) -> ErrorOr<void> {
        return m_context_list.with([&description, callback](auto& context_list) -> ErrorOr<void> {
            for (auto& context : context_list) {
                if (&context.associated_description == &description) {
                    return callback(context);
                }
            }

            // Calling ioctl on a OpenFileDescription that wasn't attached should be impossible.
            VERIFY_NOT_REACHED();
        });
    };

    switch (request) {
    case V3D_ALLOCATE_BUFFER: {
        auto buffer_create_info = TRY(copy_typed_from_user(static_ptr_cast<V3DBuffer const*>(arg)));

        TRY(with_context_for_description(description, [&buffer_create_info, this](Context& context) { return allocate_buffer(context, buffer_create_info); }));
        TRY(copy_to_user(static_ptr_cast<V3DBuffer*>(arg), &buffer_create_info));

        return {};
    }

    case V3D_FREE_BUFFER: {
        auto buffer_gpu_vaddr = arg.ptr();
        return with_context_for_description(description, [buffer_gpu_vaddr, this](Context& context) { return free_buffer(context, buffer_gpu_vaddr); });
    }

    case V3D_SUBMIT_JOB: {
        auto job = TRY(copy_typed_from_user(static_ptr_cast<V3DJob const*>(arg)));
        // dbgln("V3D: Submit job");

        Vector<V3D::AddressRange> address_ranges_to_map;
        PageTable* page_table;
        with_context_for_description(description, [&address_ranges_to_map, &page_table](Context& context) -> ErrorOr<void> {
            page_table = &context.page_table;
            for (auto& buffer : context.buffers) {
                TRY(address_ranges_to_map.try_empend(*buffer.vmobject, buffer.gpu_vaddr));
            }

            return {};
        }).release_value_but_fixme_should_propagate_errors();

        auto result = m_v3d->submit_job(*page_table, job);

        if (result.is_error()) {
            dbgln("SUBMIT_JOB args:");
            dbgln("  tile_state_data_array_base_address={:#08x}", job.tile_state_data_array_base_address);
            dbgln("  tile_allocation_memory_base_address={:#08x}", job.tile_allocation_memory_base_address);
            dbgln("  tile_allocation_memory_size={:#08x}", job.tile_allocation_memory_size);
            dbgln("  binning_control_list_address={:#08x}", job.binning_control_list_address);
            dbgln("  binning_control_list_size={:#08x}", job.binning_control_list_size);
            dbgln("  rendering_control_list_address={:#08x}", job.rendering_control_list_address);
            dbgln("  rendering_control_list_size={:#08x}", job.rendering_control_list_size);

            dbgln("Address ranges that were mapped:");
            for (auto const& address_range : address_ranges_to_map) {
                dbgln("  V{:p}-{:p}", address_range.gpu_vaddr, address_range.gpu_vaddr + address_range.vmobject.size());
            }
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

    auto with_context_for_description = [this](OpenFileDescription& description, auto&& callback) -> ErrorOr<void> {
        return m_context_list.with([&description, callback](auto& context_list) -> ErrorOr<void> {
            for (auto& context : context_list) {
                if (&context.associated_description == &description) {
                    return callback(context);
                }
            }

            // Calling mmap on a OpenFileDescription that wasn't attached should be impossible.
            VERIFY_NOT_REACHED();
        });
    };

    LockRefPtr<Memory::VMObject> vmobject;

    TRY(with_context_for_description(description, [offset, &vmobject](Context& context) -> ErrorOr<void> {
        for (auto const& buffer : context.buffers) {
            if (buffer.gpu_vaddr == offset) {
                vmobject = buffer.vmobject;
                return {};
            }
        }

        return EFAULT;
    }));

    offset = 0;

    return VMObjectAndMemoryType {
        .vmobject = vmobject.release_nonnull(),
        .memory_type = Memory::MemoryType::NonCacheable,
    };
}

GPU3DDevice::GPU3DDevice(V3D& v3d)
    : CharacterDevice(MajorAllocation::CharacterDeviceFamily::GPURender, 0) // XXX: Don't hardcode minor id
    , m_v3d(v3d)
{
}

ErrorOr<void> GPU3DDevice::allocate_buffer(Context& context, V3DBuffer& buffer_create_info)
{
    // XXX: Check additionally V3D page size.
    if ((buffer_create_info.size % PAGE_SIZE) != 0)
        return EINVAL;

    // We need to use AllocateNow since we don't want to (and can't even) lazily page in data for the GPU.
    auto vmobject = TRY(Memory::AnonymousVMObject::try_create_with_size(buffer_create_info.size, AllocationStrategy::AllocateNow));
    // auto vmobject = TRY(Memory::AnonymousVMObject::try_create_physically_contiguous_with_size(buffer_create_info.size, Memory::MemoryType::NonCacheable));

    auto region = TRY(Memory::Region::create_unbacked());
    TRY(context.region_tree.place_anywhere(*region, Memory::RandomizeVirtualAddress::No, buffer_create_info.size, 4096));

    auto gpu_vaddr = region->vaddr();

    buffer_create_info.gpu_virtual_address = gpu_vaddr.get();
    // buffer_create_info.address = vmobject->physical_pages()[0]->paddr().get(); // XXX: Add IOMMU support

    // FIXME: This requires special handling if V3D page size != PAGE_SIZE.

    // dbgln("V3D: create buffer: id={}, mmap_offset={:#x}, address={:#x}, size={:#x}", buffer_create_info.id, buffer_create_info.mmap_offset, buffer_create_info.address, buffer_create_info.size);

    m_v3d->map_buffer(context.page_table, gpu_vaddr.get(), vmobject);

    context.buffers.try_append({
                                   .vmobject = move(vmobject),
                                   .gpu_vaddr = static_cast<u32>(gpu_vaddr.get()),
                                   .region = move(region),
                               })
        .release_value_but_fixme_should_propagate_errors();

    return {};
}

ErrorOr<void> GPU3DDevice::free_buffer(Context& context, FlatPtr buffer_gpu_vaddr)
{
    // XXX: Should we allow freeing buffers if they are still mmap()ed?
    //      If we allow that, the buffer will stay alive because the Region will keep the refcount of the VMObject nonzero.
    auto buffer_index = context.buffers.find_first_index_if([buffer_gpu_vaddr](Context::Buffer const& buffer) { return buffer.gpu_vaddr == buffer_gpu_vaddr; });
    if (!buffer_index.has_value())
        return EINVAL;

    auto const& buffer = context.buffers[*buffer_index];

    m_v3d->unmap_buffer(context.page_table, buffer.gpu_vaddr, *buffer.vmobject);

    context.region_tree.remove(*buffer.region);
    context.buffers.remove(*buffer_index);

    return {};
}

}
