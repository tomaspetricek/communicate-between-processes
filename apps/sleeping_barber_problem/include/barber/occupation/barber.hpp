#ifndef BARBER_OCCUPATION_BARBER_HPP
#define BARBER_OCCUPATION_BARBER_HPP

#include <atomic>
#include <print>
#include <thread>

#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/error_code.hpp"

#include "barber/customer_queue.hpp"

namespace barber::occupation
{
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
                std::println("shop closed");
                break;
            }
            if (!customer_waiting)
            {
                std::println("failed to waiting for customer waiting due to: {}",
                             unix::to_string(customer_waiting.error()).data());
                return false;
            }
            const auto customer = customer_queue.pop();
            std::println("trimming customer: {} hair", customer);
            std::this_thread::sleep_for(std::chrono::milliseconds{get_haircut_duration()});
            served_customers.fetch_add(1, std::memory_order_relaxed);
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
}

#endif // BARBER_OCCUPATION_BARBER_HPP