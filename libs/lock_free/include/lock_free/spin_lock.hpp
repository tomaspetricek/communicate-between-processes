#ifndef LOCK_FREE_SPIN_LOCK_HPP
#define LOCK_FREE_SPIN_LOCK_HPP

#include <atomic>

namespace lock_free
{
    // src: https://medium.com/@amishav/implementing-a-spinlock-in-c-8078ec584efc
    class spin_lock
    {
    public:
        void lock() noexcept
        {
            while (flag_.test_and_set(std::memory_order_acquire))
            {
            }
        }

        void unlock() noexcept { flag_.clear(std::memory_order_release); }

    private:
        std::atomic_flag flag_{ATOMIC_FLAG_INIT};
    };
} // namespace lock_free

#endif // LOCK_FREE_SPIN_LOCK_HPP