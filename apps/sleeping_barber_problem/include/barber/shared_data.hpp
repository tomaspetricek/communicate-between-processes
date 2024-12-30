#ifndef BARBER_SHARED_DATA_HPP
#define BARBER_SHARED_DATA_HPP

#include <atomic>
#include <stdint.h>
#include <cstddef>

#include "barber/customer_queue.hpp"

namespace barber
{
struct shared_data
{
    std::atomic<bool> shop_closed{false};
    std::atomic<customer_t> next_customer_id{0};
    std::atomic<std::int32_t> served_customers{0};
    std::atomic<std::int32_t> refused_customers{0};
    customer_queue_t customer_queue;
};
} // namespace barber

#endif // BARBER_SHARED_DATA_HPP