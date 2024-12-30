#ifndef BARBER_CUSTOMER_QUEUE_HPP
#define BARBER_CUSTOMER_QUEUE_HPP

#include <cstddef>
#include <stdint.h>

#include "lock_free/ring_buffer.hpp"

namespace barber
{
    using customer_t = std::int32_t;
    constexpr std::size_t waiting_chair_count = 5;
    using customer_queue_t =
        lock_free::ring_buffer<customer_t, waiting_chair_count>;
} // namespace barber

#endif // BARBER_CUSTOMER_QUEUE_HPP