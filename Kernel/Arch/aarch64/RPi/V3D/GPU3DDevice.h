/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Kernel/Devices/CharacterDevice.h>
#include <Kernel/Memory/AnonymousVMObject.h>

namespace Kernel::RPi::V3D {

class V3D;
class GPU3DDevice final : public CharacterDevice {
    friend class Device;

public:
    static ErrorOr<NonnullRefPtr<GPU3DDevice>> create(V3D&);

    virtual bool can_read(OpenFileDescription const&, u64) const override { return false; }
    virtual bool can_write(OpenFileDescription const&, u64) const override { return false; }

    virtual ErrorOr<void> attach(OpenFileDescription&) override;

    virtual ErrorOr<size_t> read(OpenFileDescription&, u64, UserOrKernelBuffer&, size_t) override { return ENOTSUP; }
    virtual ErrorOr<size_t> write(OpenFileDescription&, u64, UserOrKernelBuffer const&, size_t) override { return ENOTSUP; }
    virtual ErrorOr<void> ioctl(OpenFileDescription&, unsigned request, Userspace<void*> arg) override;
    virtual ErrorOr<VMObjectAndMemoryType> vmobject_and_memory_type_for_mmap(OpenFileDescription&, Memory::VirtualRange const&, u64&, bool) override;

    virtual StringView class_name() const override { return "V3D::GPU3DDevice"sv; }

private:
    GPU3DDevice(V3D&);

    struct PerContextState : public AtomicRefCounted<PerContextState> {
        friend class GPU3DDevice;

        PerContextState(OpenFileDescription& file_description)
            : attached_file_description(file_description)
        {
        }

        struct Buffer {
            NonnullLockRefPtr<Memory::AnonymousVMObject> vmobject;
            u64 mmap_offset;
        };

        Vector<Buffer> buffers;

        off_t next_buffer_mmap_offset { 0 };

        // NOTE: We clean this whole object when the file description is closed, therefore we need to hold
        // a raw reference here instead of a strong reference pointer (e.g. RefPtr, which will make it
        // possible to leak the attached OpenFileDescription for a context in this device).
        OpenFileDescription& attached_file_description;

        IntrusiveListNode<PerContextState, NonnullRefPtr<PerContextState>> list_node;
    };

    using ContextList = IntrusiveListRelaxedConst<&PerContextState::list_node>;

    SpinlockProtected<ContextList, LockRank::None> m_context_state_list;
    NonnullRefPtr<V3D> m_v3d;
};

}
