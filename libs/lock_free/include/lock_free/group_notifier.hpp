#ifndef LOCK_FREE_GROUP_NOTIFIER_HPP
#define LOCK_FREE_GROUP_NOTIFIER_HPP

#include <atomic>
#include <cassert>
#include <cstdint>

namespace lock_free
{
    class group_notifier
    {
        using value_type = std::int32_t;

        std::atomic<value_type> counter_{0};
        value_type group_size_{0};

    public:
        explicit group_notifier(value_type init_value, value_type group_size) noexcept
            : counter_{init_value}, group_size_{group_size}
        {
            assert(group_size > 0);
            assert(init_value >= 0);
            assert(init_value <= group_size);
        }

        void wait_for_one() noexcept
        {
            value_type expected;

            do
            {
                expected = counter_.load(std::memory_order_relaxed);

                if (expected <= 0)
                {
                    counter_.wait(0, std::memory_order_acquire);
                }
            } while (!counter_.compare_exchange_weak(expected, expected - 1,
                                                     std::memory_order_acquire,
                                                     std::memory_order_relaxed));
        }

        void notify_all() noexcept
        {
            counter_.fetch_add(group_size_, std::memory_order_release);
            counter_.notify_all();
        }

        void notify_one() noexcept
        {
            counter_.fetch_add(1, std::memory_order_release);
            counter_.notify_one();
        }
    };
} // namespace lock_free

#endif // LOCK_FREE_GROUP_NOTIFIER_HPP