/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Forward.h>
#include <Kernel/Devices/Storage/NVMe/AdminCommandset.h>
#include <Kernel/Devices/Storage/NVMe/DataStructures.h>
#include <Kernel/Memory/DMA.h>

namespace Kernel::NVMe {

// FIXME: Maybe use non-physically contiguous I/O Queues. This is supported by the controller if CAP.CQR == 0.
//        Adming Queues always have to be physically contiguous.

union SubmissionQueueEntry {
    AdminOrIOCommandSubmissionQueueEntry common;
    IdentifyCommand identify_command;
};

union CompletionQueueEntry {
    AdminOrIOCommandCompletionQueueEntry common;
};

class Queue {
public:
    u64 dma_base_address() { return m_dma_buffer.bus_address(); }
    size_t size() const { return m_size; }

protected:
    Queue(Memory::ContiguousDMABuffer, size_t queue_size, size_t queue_entry_size);

    template<typename T>
    T const& entry(size_t index) const
    {
        auto vaddr = m_dma_buffer.virtual_address().offset(index * m_entry_size);
        return *reinterpret_cast<T const*>(vaddr.as_ptr());
    }

    template<typename T>
    T& entry(size_t index)
    {
        auto vaddr = m_dma_buffer.virtual_address().offset(index * m_entry_size);
        return *reinterpret_cast<T*>(vaddr.as_ptr());
    }

    size_t m_size { 0 };
    size_t m_entry_size { 0 };
    Memory::ContiguousDMABuffer m_dma_buffer;
};

class SubmissionQueue : public Queue {
public:
    SubmissionQueue(Memory::ContiguousDMABuffer, size_t queue_size, size_t queue_entry_size);

    ErrorOr<void> submit(SubmissionQueueEntry);

    bool is_full() const;

    void update_head_index(size_t);

    size_t current_tail_index() const { return m_tail_index; }

private:
    size_t m_head_index { 0 }; // Managed by the controller, therefore it might be behind the actual value.
    size_t m_tail_index { 0 }; // Managed by us.
};

class CompletionQueue : public Queue {
public:
    CompletionQueue(Memory::ContiguousDMABuffer, size_t queue_size, size_t queue_entry_size);

    // Or consume/complete?
    ErrorOr<CompletionQueueEntry> dequeue();

    bool is_empty() const;

    size_t current_head_index() const { return m_head_index; }

private:
    // Initially all phase tag bits are cleared to zero.
    // When the controller completes commands, it inverts the phase bit, so we expect it to be '1' initially.
    bool m_expected_phase_tag { true };

    size_t m_head_index { 0 }; // Managed by us.
};

}
