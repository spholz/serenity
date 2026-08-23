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

// XXX: Put in namespace, same for Buffer, and everything else

class ControlList {
public:
    ControlList() = default;

    static ErrorOr<ControlList> create(u32 size)
    {
        auto buffer_object = TRY(Buffer::create(size));
        void* mapped_buffer = TRY(buffer_object.map());

        auto buffer = Bytes { mapped_buffer, buffer_object.size() };
        buffer.fill(0);

        return ControlList(move(buffer_object), buffer);
    }

    template<typename T>
    void append(T const& packet)
    {
        VERIFY(m_offset + sizeof(T) <= m_data.size());
        memcpy(m_data.data() + m_offset, &packet, sizeof(T));
        m_offset += sizeof(T);
    }

    // XXX: Or clear_with_capacity()?
    void clear()
    {
        m_data.fill(0);
        m_offset = 0;
    }

    Bytes data() const
    {
        return m_data.slice(0, m_offset);
    }

    Buffer const& buffer() const
    {
        return m_buffer;
    }

private:
    ControlList(Buffer buffer, Bytes data)
        : m_buffer(move(buffer))
        , m_data(data)
    {
    }

    Buffer m_buffer;
    Span<u8> m_data;
    size_t m_offset = 0;
};

template<>
struct AK::Formatter<ControlList> : Formatter<StringView> {
    ErrorOr<void> format(FormatBuilder& builder, ControlList const& control_list)
    {
        builder.builder().appendff("ControlList {{ buffer_object = {} }}", control_list.buffer());
        return {};
    }
};
