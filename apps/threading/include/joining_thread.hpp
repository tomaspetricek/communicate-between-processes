#ifndef THREADING_JOINING_THREAD_HPP
#define THREADING_JOINING_THREAD_HPP

#include <thread>
#include <utility>

class joining_thread
{
    std::thread thread_;

public:
    joining_thread() noexcept = default;

    template <class Callable, class... Args>
    explicit joining_thread(Callable &&call, Args &&...args)
        : thread_(std::forward<Callable>(call), std::forward<Args>(args)...) {}

    explicit joining_thread(std::thread thread) noexcept
        : thread_(std::move(thread)) {}

    joining_thread(std::thread &&thread) noexcept : thread_(std::move(thread)) {}

    joining_thread &operator=(joining_thread &&other) noexcept
    {
        if (joinable())
        {
            join();
        }
        thread_ = std::move(other.thread_);
        return *this;
    }

    joining_thread &operator=(joining_thread other) noexcept
    {
        if (joinable())
        {
            join();
        }
        thread_ = std::move(other.thread_);
        return *this;
    }

    ~joining_thread() noexcept
    {
        if (joinable())
        {
            join();
        }
    }

    void swap(joining_thread &other) noexcept { thread_.swap(other.thread_); }

    std::thread::id get_id() const noexcept { return thread_.get_id(); }

    bool joinable() const noexcept { return thread_.joinable(); }

    void join() { thread_.join(); }

    void detach() { thread_.detach(); }

    std::thread &as_thread() noexcept { return thread_; }

    const std::thread &as_thread() const noexcept { return thread_; }
};

#endif // THREADING_JOINING_THREAD_HPP