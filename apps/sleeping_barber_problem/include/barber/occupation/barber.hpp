#ifndef BARBER_OCCUPATION_BARBER_HPP
#define BARBER_OCCUPATION_BARBER_HPP

#include <atomic>
#include <print>
#include <thread>

#include "unix/error_code.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"

#include "barber/customer_queue.hpp"

namespace barber::occupation
{
    void trim_hair(customer_t customer, std::chrono::milliseconds trimming_duration,
                   std::atomic<std::int32_t> &served_customers) noexcept
    {
        std::println("trimming customer: {} hair", customer);
        std::this_thread::sleep_for(trimming_duration);
        served_customers.fetch_add(1, std::memory_order_relaxed);
    }

    template <class GetSleepingDuration>
    bool serve_customer(
        const unix::system_v::ipc::group_notifier &customer_waiting_notifier,
        const unix::system_v::ipc::group_notifier &empty_chair_notifier,
        customer_queue_t &customer_queue, const std::atomic<bool> &shop_closed,
        std::atomic<std::int32_t> &served_customers,
        GetSleepingDuration &get_haircut_duration) noexcept
    {
        while (true)
        {
            std::println("waiting for customer");
            const auto customer_waiting = customer_waiting_notifier.wait_for_one();

            if (shop_closed.load(std::memory_order_relaxed))
            {
                std::println("shop is closing, finishing serving last customers");

                while (!customer_queue.empty())
                {
                    const auto possible_customer = customer_queue.try_pop();

                    if (possible_customer)
                    {
                        trim_hair(possible_customer.value(),
                                  std::chrono::milliseconds{get_haircut_duration()},
                                  served_customers);
                    }
                }
                break;
            }
            if (!customer_waiting)
            {
                std::println("failed to waiting for customer waiting due to: {}",
                             unix::to_string(customer_waiting.error()).data());
                return false;
            }
            const auto customer = customer_queue.pop();
            trim_hair(customer, std::chrono::milliseconds{get_haircut_duration()},
                      served_customers);
            const auto &empty_chair = empty_chair_notifier.notify_one();

            if (!empty_chair)
            {
                std::println("failed to notify about empty chair due to: {}",
                             unix::to_string(empty_chair.error()).data());
                return false;
            }
        }
        return true;
    }
} // namespace barber::occupation

#endif // BARBER_OCCUPATION_BARBER_HPP