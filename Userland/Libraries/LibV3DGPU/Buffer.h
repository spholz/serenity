/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Format.h>
#include <AK/Forward.h>
#include <AK/Noncopyable.h>
#include <AK/StringBuilder.h>
#include <AK/Types.h>
#include <sys/types.h>

class Buffer {
    AK_MAKE_NONCOPYABLE(Buffer);

public:
    ~Buffer();

    // XXX: Remove this constructor.
    Buffer() { }

    Buffer(Buffer&& other)
        : m_gpu_virtual_address(other.m_gpu_virtual_address)
        , m_size(other.m_size)
        , m_mmap_address(other.m_mmap_address)
    {
        other.m_gpu_virtual_address = 0;
        other.m_size = 0;
        other.m_mmap_address = nullptr;
    }

    Buffer& operator=(Buffer&& other)
    {
        if (this != &other) {
            this->~Buffer();
            m_size = exchange(other.m_size, 0);
            m_gpu_virtual_address = exchange(other.m_gpu_virtual_address, 0);
            m_mmap_address = exchange(other.m_mmap_address, nullptr);
        }
        return *this;
    }

    static ErrorOr<Buffer> create(u32 size);

    ErrorOr<void*> map();

    u32 size() const { return m_size; }
    u32 gpu_virtual_address() const { return m_gpu_virtual_address; }

private:
    Buffer(u32 address, u32 size)
        : m_gpu_virtual_address(address)
        , m_size(size)
    {
    }

    u32 m_gpu_virtual_address { 0 };
    u32 m_size { 0 };

    void* m_mmap_address { nullptr };
};

template<>
struct AK::Formatter<Buffer> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, Buffer const& buffer_object)
    {
        builder.builder().appendff("BufferObject {{ size = {:#x}, GPU address = {:#08x} }}",
            buffer_object.size(), buffer_object.gpu_virtual_address());
        return {};
    }
};
