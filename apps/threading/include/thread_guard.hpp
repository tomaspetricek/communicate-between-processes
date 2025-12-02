#ifndef THREADING_THREAD_GUARD_HPP
#define THREADING_THREAD_GUARD_HPP

#include <thread>

class thread_guard
{
    std::thread &thread_;

public:
    explicit thread_guard(std::thread &thread) : thread_(thread) {}

    ~thread_guard()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }
    // forbid copying
    thread_guard(const thread_guard &) = delete;
    thread_guard operator=(const thread_guard &) = delete;
};

#endif // THREADING_THREAD_GUARD_HPP