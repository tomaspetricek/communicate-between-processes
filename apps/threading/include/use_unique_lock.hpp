#ifndef THREADING_USE_UNIQUE_LOCK_HPP
#define THREADING_USE_UNIQUE_LOCK_HPP

#include <mutex>

class some_big_object
{
};
void swap(some_big_object &lhs, some_big_object &rhs) {}

class Y
{
    some_big_object some_detail;
    std::mutex m;

public:
    Y(const some_big_object &some_detail) : some_detail(some_detail) {}

    friend void swap(Y &rhs, Y &lhs)
    {
        if (&rhs == &lhs)
        {
            return;
        }
        std::unique_lock lock_rhs(rhs.m, std::defer_lock);
        std::unique_lock lock_lhs(lhs.m, std::defer_lock);
        std::lock(lock_rhs, lock_lhs);
        swap(rhs.some_detail, lhs.some_detail);
    }
};

#endif // THREADING_USE_UNIQUE_LOCK_HPP