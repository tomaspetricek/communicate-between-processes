#ifndef BUFFERING_SHARED_DATA_HPP
#define BUFFERING_SHARED_DATA_HPP

#include <atomic>

#include "buffering/message_queue.hpp"

namespace buffering
{
    struct shared_data
    {
        std::atomic<bool> done_flag{false};
        std::atomic<std::int32_t> produced_message_count{0};
        std::atomic<std::int32_t> consumed_message_count{0};
        buffering::message_queue_t message_queue;
    };
} // namespace

#endif // BUFFERING_SHARED_DATA_HPP