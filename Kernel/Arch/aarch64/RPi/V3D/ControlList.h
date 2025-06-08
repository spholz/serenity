/*
 * Copyright (c) 2025, Sönke Holz <sholz8530@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Span.h>
#include <AK/Types.h>
#include <Kernel/Library/MiniStdLib.h>

namespace Kernel::RPi::V3D {

class ControlList {
public:
    ControlList(Span<u8> buffer)
        : m_buffer(buffer)
    {
    }

    template<typename T>
    void append(T const& packet)
    {
        VERIFY(m_offset + sizeof(T) <= m_buffer.size());
        memcpy(m_buffer.data() + m_offset, &packet, sizeof(T));
        m_offset += sizeof(T);
    }

    Span<u8> data() const
    {
        return m_buffer.slice(0, m_offset);
    }

private:
    Span<u8> m_buffer;
    size_t m_offset = 0;
};

}
