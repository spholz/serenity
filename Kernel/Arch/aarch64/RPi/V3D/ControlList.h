/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Span.h>
#include <AK/Types.h>

#include "Buffer.h"

#ifdef KERNEL
namespace Kernel::RPi::V3D {
#endif

class ControlList {
public:
    ControlList() = default;

    ControlList(BufferObject bo, Span<u8> buffer)
        : m_bo(bo)
        , m_buffer(buffer)
    {
    }

    template<typename T>
    void append(T const& packet)
    {
        VERIFY(m_offset + sizeof(T) <= m_buffer.size());
        __builtin_memcpy(m_buffer.data() + m_offset, &packet, sizeof(T));
        m_offset += sizeof(T);
    }

    Span<u8> data() const
    {
        return m_buffer.slice(0, m_offset);
    }

    BufferObject const& bo() const
    {
        return m_bo;
    }

private:
    BufferObject m_bo;
    Span<u8> m_buffer;
    size_t m_offset = 0;
};

#ifdef KERNEL
}
#endif
