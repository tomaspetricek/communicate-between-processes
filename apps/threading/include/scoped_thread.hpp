#ifndef THREADING_SCOPED_THREAD_HPP
#define THREADING_SCOPED_THREAD_HPP

#include <thread>
#include <exception>

class scoped_thread
{
    std::thread thread_;

public:
    explicit scoped_thread(std::thread thread) : thread_(std::move(thread))
    {
        if (!thread_.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread() { thread_.join(); }
    // forbid copying
    scoped_thread(const scoped_thread &) = delete;
    scoped_thread operator=(const scoped_thread &) = delete;
};

#endif // THREADING_SCOPED_THREAD_HPP