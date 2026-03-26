/*
 * Copyright (c) 2026, Sönke Holz <soenke.holz@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <Kernel/Devices/Storage/NVMe/Queue.h>

namespace Kernel::NVMe {

Queue::Queue(Memory::ContiguousDMABuffer dma_buffer, size_t queue_size, size_t queue_entry_size)
    : m_size(queue_size)
    , m_entry_size(queue_entry_size)
    , m_dma_buffer(move(dma_buffer))
{
}

SubmissionQueue::SubmissionQueue(Memory::ContiguousDMABuffer dma_buffer, size_t queue_size, size_t queue_entry_size)
    : Queue(move(dma_buffer), queue_size, queue_entry_size)
{
}

ErrorOr<void> SubmissionQueue::submit(SubmissionQueueEntry new_entry)
{
    if (is_full())
        return ENOBUFS;

    entry<SubmissionQueueEntry>(m_tail_index) = new_entry;

    m_tail_index++;

    if (m_tail_index >= m_size)
        m_tail_index = 0;

    return {};
}

bool SubmissionQueue::is_full() const
{
    // 3.3.1.5 Full Queue
    // "The queue is Full when the Head equals one more than the Tail."

    return m_head_index == ((m_tail_index + 1) % m_size);
}

void SubmissionQueue::update_head_index(size_t new_head_index)
{
    // XXX: Maybe add sanity checks here?
    m_head_index = new_head_index;
}

CompletionQueue::CompletionQueue(Memory::ContiguousDMABuffer dma_buffer, size_t queue_size, size_t queue_entry_size)
    : Queue(move(dma_buffer), queue_size, queue_entry_size)
{
}

ErrorOr<CompletionQueueEntry> CompletionQueue::dequeue()
{
    if (is_empty())
        return EAGAIN;

    auto dequeued_entry = entry<CompletionQueueEntry>(m_head_index);

    m_head_index++;

    if (m_head_index >= m_size) {
        m_expected_phase_tag = !m_expected_phase_tag;
        m_head_index = 0;
    }

    return dequeued_entry;
}

bool CompletionQueue::is_empty() const
{
    // 3.3.1.4 Empty Queue
    // "The queue is Empty when the Head entry pointer equals the Tail entry pointer."

    // 3.3.1.2 Queue Usage
    // "A host checks completion queue entry Phase Tag (P) bits in memory to determine whether new completion
    //  queue entries have been posted (refer to section 4.2.4). The Completion Queue Tail pointer is only used
    //  internally by the controller and is not visible to the host."

    return entry<AdminOrIOCommandCompletionQueueEntry>(m_head_index).phase_tag != m_expected_phase_tag;
}

}
