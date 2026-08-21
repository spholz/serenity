/*
 * Copyright (c) 2025, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Error.h>
#include <AK/Format.h>
#include <AK/Span.h>
#include <AK/StringBuilder.h>
#include <AK/Types.h>

#include "Buffer.h"

// XXX: Put in namespace, same for BufferObject, and everything else

class ControlList {
public:
    ControlList() = default;

    static ErrorOr<ControlList> create(u32 size)
    {
        auto buffer_object = TRY(BufferObject::create(size));
        void* mapped_buffer = TRY(buffer_object.map());

        auto buffer = Bytes { mapped_buffer, buffer_object.size() };
        buffer.fill(0);

        return ControlList(move(buffer_object), buffer);
    }

    template<typename T>
    void append(T const& packet)
    {
        VERIFY(m_offset + sizeof(T) <= m_buffer.size());
        memcpy(m_buffer.data() + m_offset, &packet, sizeof(T));
        m_offset += sizeof(T);
    }

    // XXX: Or clear_with_capacity()?
    void clear()
    {
        m_buffer.fill(0);
        m_offset = 0;
    }

    Bytes data() const
    {
        return m_buffer.slice(0, m_offset);
    }

    BufferObject const& buffer_object() const
    {
        return m_bo;
    }

private:
    ControlList(BufferObject bo, Bytes buffer)
        : m_bo(move(bo))
        , m_buffer(buffer)
    {
    }

    BufferObject m_bo;
    Span<u8> m_buffer;
    size_t m_offset = 0;
};

template<>
struct AK::Formatter<ControlList> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, ControlList const& control_list)
    {
        builder.builder().appendff("ControlList {{ buffer_object = {} }}", control_list.buffer_object());
        return {};
    }
};
