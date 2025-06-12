/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/API/Ioctl.h>
#include <Kernel/API/V3D.h>
#include <Kernel/Arch/aarch64/RPi/V3D/GPU3DDevice.h>
#include <Kernel/Arch/aarch64/RPi/V3D/V3D.h>

namespace Kernel::RPi::V3D {

ErrorOr<NonnullRefPtr<GPU3DDevice>> GPU3DDevice::create(V3D& v3d)
{
    return TRY(Device::try_create_device<GPU3DDevice>(v3d));
}

ErrorOr<void> GPU3DDevice::attach(OpenFileDescription& description)
{
    dbgln("V3D attach");

    auto context = make_ref_counted<PerContextState>(description);

    m_context_state_list.with([&context](auto& context_state_list) {
        context_state_list.append(context);
    });

    return CharacterDevice::attach(description);
}

ErrorOr<void> GPU3DDevice::ioctl(OpenFileDescription& description, unsigned request, Userspace<void*> arg)
{
    auto with_context_for_description = [this](OpenFileDescription& description, auto&& callback) -> ErrorOr<void> {
        return m_context_state_list.with([&description, callback](auto& context_state_list) -> ErrorOr<void> {
            for (auto& context : context_state_list) {
                if (&context.attached_file_description == &description) {
                    return callback(context);
                }
            }

            // Calling ioctl on a OpenFileDescription that wasn't attached should be impossible.
            VERIFY_NOT_REACHED();
        });
    };

    dbgln("V3D: ioctl({}, ...)", request);

    switch (request) {
    case V3D_CREATE_BUFFER: {
        auto buffer_create_info = TRY(copy_typed_from_user(static_ptr_cast<V3DBuffer const*>(arg)));

        if ((buffer_create_info.size % PAGE_SIZE) != 0)
            return EINVAL;

        return with_context_for_description(description, [&buffer_create_info, arg](PerContextState& context) -> ErrorOr<void> {
            auto vmobject = TRY(Memory::AnonymousVMObject::try_create_physically_contiguous_with_size(buffer_create_info.size, Memory::MemoryType::NonCacheable));

            buffer_create_info.id = context.buffers.size();
            buffer_create_info.mmap_offset = context.next_buffer_mmap_offset,
            buffer_create_info.address = vmobject->physical_pages()[0]->paddr().get(); // XXX: Add IOMMU support

            dbgln("V3D: create buffer: id={}, mmap_offset={:#x}, address={:#x}, size={:#x}", buffer_create_info.id, buffer_create_info.mmap_offset, buffer_create_info.address, buffer_create_info.size);
            TRY(copy_to_user(static_ptr_cast<V3DBuffer*>(arg), &buffer_create_info));

            context.buffers.append({
                .vmobject = move(vmobject),
                .mmap_offset = buffer_create_info.mmap_offset,
            });

            context.next_buffer_mmap_offset += buffer_create_info.size;

            return {};
        });
    }

    case V3D_SUBMIT_JOB: {
        auto job = TRY(copy_typed_from_user(static_ptr_cast<V3DJob const*>(arg)));
        dbgln("V3D: Submit job");
        dbgln("  tile_state_data_array_base_address={}", job.tile_state_data_array_base_address);
        dbgln("  tile_allocation_memory_base_address={}", job.tile_allocation_memory_base_address);
        dbgln("  tile_allocation_memory_size={}", job.tile_allocation_memory_size);
        dbgln("  binning_control_list_address={}", job.binning_control_list_address);
        dbgln("  binning_control_list_size={}", job.binning_control_list_size);
        dbgln("  rendering_control_list_address={}", job.rendering_control_list_address);
        dbgln("  rendering_control_list_size={}", job.rendering_control_list_size);

        m_v3d->submit_job(job);

        return {};
    }
    }

    return EINVAL;
}

ErrorOr<File::VMObjectAndMemoryType> GPU3DDevice::vmobject_and_memory_type_for_mmap(OpenFileDescription& description, Memory::VirtualRange const&, u64& offset, bool)
{
    if ((offset % PAGE_SIZE) != 0)
        return EINVAL;

    auto with_context_for_description = [this](OpenFileDescription& description, auto&& callback) -> ErrorOr<void> {
        return m_context_state_list.with([&description, callback](auto& context_state_list) -> ErrorOr<void> {
            for (auto& context : context_state_list) {
                if (&context.attached_file_description == &description) {
                    return callback(context);
                }
            }

            // Calling mmap on a OpenFileDescription that wasn't attached should be impossible.
            VERIFY_NOT_REACHED();
        });
    };

    LockRefPtr<Memory::VMObject> vmobject;

    TRY(with_context_for_description(description, [offset, &vmobject](PerContextState& context) -> ErrorOr<void> {
        for (auto const& buffer : context.buffers) {
            if (buffer.mmap_offset == offset) {
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

}
