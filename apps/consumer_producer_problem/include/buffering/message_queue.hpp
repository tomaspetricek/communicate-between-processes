#ifndef BUFFERING_MESSAGE_QUEUE_HPP
#define BUFFERING_MESSAGE_QUEUE_HPP

#include <array>
#include <cstddef>

#include "lock_free/ring_buffer.hpp"

namespace buffering
{
    constexpr std::size_t message_capacity = 256;
    constexpr std::size_t message_queue_capacity = 32;
    using message_t = std::array<char, message_capacity>;
    using message_queue_t =
        lock_free::ring_buffer<message_t, message_queue_capacity>;
} // namespace

#endif // BUFFERING_MESSAGE_QUEUE_HPP