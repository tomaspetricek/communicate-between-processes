#ifndef BARBER_OCCUPATION_CUSTOMER_GENERATOR_HPP
#define BARBER_OCCUPATION_CUSTOMER_GENERATOR_HPP

#include <atomic>
#include <cstddef>
#include <print>
#include <thread>

#include "unix/error_code.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"

#include "barber/customer_queue.hpp"

namespace barber::occupation
{
    template <class GetSleepingDuration>
    bool generate_customers(
        const unix::system_v::ipc::group_notifier &customer_waiting_notifier,
        const unix::system_v::ipc::group_notifier &empty_chair_notifier,
        customer_queue_t &customer_queue, std::atomic<customer_t> &next_customer_id,
        const std::atomic<bool> &shop_closed,
        std::atomic<std::int32_t> &refused_customers,
        GetSleepingDuration &get_customer_arrival_duration) noexcept
    {
        while (true)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds{get_customer_arrival_duration()});
            const auto customer = next_customer_id++;
            const auto empty_chair = empty_chair_notifier.try_waiting_for_one();

            if (shop_closed.load(std::memory_order_relaxed))
            {
                std::println("shop closed");
                break;
            }
            if (!empty_chair)
            {
                if (empty_chair.error().code != EAGAIN)
                {
                    std::println("failed trying to wait for an empty chair due to: {}",
                                 unix::to_string(empty_chair.error()).data());
                    return false;
                }
                std::println("all chairs occupied, refusing customer: {}", customer);
                refused_customers.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                std::println("adding customer: {}", customer);
                customer_queue.push(customer);
                const auto customer_waiting = customer_waiting_notifier.notify_one();

                if (!customer_waiting)
                {
                    std::println("failed to notify about customer waiting due to: {}",
                                 unix::to_string(customer_waiting.error()).data());
                    return false;
                }
            }
        }
        return true;
    }
} // namespace barber::occupation

#endif // BARBER_OCCUPATION_CUSTOMER_GENERATOR_HPP