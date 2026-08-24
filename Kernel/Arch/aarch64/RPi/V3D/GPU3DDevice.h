/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/API/V3D.h>
#include <Kernel/Arch/aarch64/RPi/V3D/GPUVirtualAddress.h>
#include <Kernel/Arch/aarch64/RPi/V3D/V3D.h>
#include <Kernel/Devices/CharacterDevice.h>
#include <Kernel/Memory/AnonymousVMObject.h>
#include <Kernel/Memory/RegionTree.h>

namespace Kernel::RPi::V3D {

class GPU3DDevice final : public CharacterDevice {
    friend class Device;

public:
    static ErrorOr<NonnullRefPtr<GPU3DDevice>> create(V3D&);

    virtual bool can_read(OpenFileDescription const&, u64) const override { return false; }
    virtual bool can_write(OpenFileDescription const&, u64) const override { return false; }

    virtual ErrorOr<void> attach(OpenFileDescription&) override;
    virtual void detach(OpenFileDescription&) override;

    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override { return ENOTSUP; }
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t) override { return ENOTSUP; }
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) override;
    virtual ErrorOr<VMObjectAndMemoryType> vmobject_and_memory_type_for_mmap(OpenFileDescription&, Memory::VirtualRange const&, u64&, bool) override;

    virtual StringView class_name() const override { return "V3D::GPU3DDevice"sv; }

private:
    GPU3DDevice(V3D&);

    struct Context : public AtomicRefCounted<Context> {
        Context(OpenFileDescription& file_description, PageTable page_table)
            : page_table(move(page_table))
            , region_tree(Memory::VirtualRange { VirtualAddress { 0x1000 }, 4 * GiB })
            , associated_description(file_description)
        {
        }

        struct Buffer {
            NonnullLockRefPtr<Memory::AnonymousVMObject> vmobject;
            GPUVirtualAddress gpu_vaddr;
            NonnullOwnPtr<Memory::Region> region;
        };

        // Protects the entire Context state.
        Mutex mutex;

        Vector<Buffer> buffers;

        PageTable page_table;

        Memory::RegionTree region_tree;

        // Context is destroyed once OpenFileDescription's destructor calls File::detach() on GPU3DDevice,
        // so this struct will never outlive the lifetime of this associated OpenFileDescription.
        // It's therefore safe and necessary to use a raw reference here.
        // Otherwise we would leak a reference on the description here, causing OpenFileDescription's
        // destructor to never be called.
        OpenFileDescription& associated_description;

        IntrusiveListNode<Context, NonnullRefPtr<Context>> list_node;
    };

    ErrorOr<GPUVirtualAddress> allocate_buffer(Context&, size_t);
    ErrorOr<void> free_buffer(Context&, GPUVirtualAddress buffer_gpu_vaddr);

    Context& context_for_description(OpenFileDescription& description)
    {
        return m_context_list.with([&description](auto& context_list) -> Context& {
            for (auto& context : context_list) {
                if (&context.associated_description == &description) {
                    return context;
                }
            }

            VERIFY_NOT_REACHED();
        });
    }

    using ContextList = IntrusiveList<&Context::list_node>;

    SpinlockProtected<ContextList, LockRank::None> m_context_list;
    NonnullRefPtr<V3D> m_v3d;
};

}
