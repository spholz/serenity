/*
 * Copyright (c) 2025-2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <AK/Noncopyable.h>
#include <AK/Types.h>
#include <sys/types.h>

class BufferObject {
    AK_MAKE_NONCOPYABLE(BufferObject);

public:
    ~BufferObject();

    // XXX: Remove this constructor.
    BufferObject() { }

    BufferObject(BufferObject&& other)
        : m_handle(other.m_handle)
        , m_size(other.m_size)
        , m_address(other.m_address)
        , m_mmap_offset(other.m_mmap_offset)
        , m_mmap_address(other.m_mmap_address)
    {
        other.m_handle = 0xffff'ffff;
        other.m_size = 0;
        other.m_address = 0;
        other.m_mmap_offset = 0;
        other.m_mmap_address = nullptr;
    }

    BufferObject& operator=(BufferObject&& other)
    {
        if (this != &other) {
            this->~BufferObject();
            m_handle = exchange(other.m_handle, 0xffff'ffff);
            m_size = exchange(other.m_size, 0);
            m_address = exchange(other.m_address, 0);
            m_mmap_offset = exchange(other.m_mmap_offset, 0);
            m_mmap_address = exchange(other.m_mmap_address, nullptr);
        }
        return *this;
    }

    static ErrorOr<BufferObject> create(u32 size);

    ErrorOr<void*> map();

    u32 handle() const { return m_handle; }
    u32 size() const { return m_size; }
    u32 address() const { return m_address; }

private:
    BufferObject(u32 handle, u32 size, u32 address, off_t mmap_offset)
        : m_handle(handle)
        , m_size(size)
        , m_address(address)
        , m_mmap_offset(mmap_offset)
    {
    }

    u32 m_handle { 0xffff'ffff };
    u32 m_size { 0 };
    u32 m_address { 0 };
    off_t m_mmap_offset { 0 };

    void* m_mmap_address { nullptr };
};
