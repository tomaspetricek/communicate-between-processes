#ifndef THREADING_THREADSAFE_STACK_HPP
#define THREADING_THREADSAFE_STACK_HPP

#include <thread>
#include <utility>

#include <stack>
#include <exception>
#include <mutex>
#include <utility>

struct empty_stack : std::exception
{
    const char *what() const throw()
    {
        return "empty stack";
    }
};

template <class T>
class threadsafe_stack
{
    std::stack<T> data;
    mutable std::mutex mutex;

public:
    threadsafe_stack() = default;

    threadsafe_stack(const threadsafe_stack &other)
    {
        std::lock_guard lock(other.mutex);
        data = other.data;
    }

    threadsafe_stack &operator=(const threadsafe_stack &) = delete;

    void push(T value)
    {
        std::lock_guard lock(mutex);
        data.push(std::move(value));
    }

    std::shared_ptr<T> pop()
    {
        std::lock_guard lock(mutex);

        if (data.empty())
        {
            throw empty_stack();
        }
        const auto res = std::make_shared(data.top());
        data.pop();
        return res;
    }

    void pop(T &value)
    {
        std::lock_guard lock(mutex);

        if (data.empty())
        {
            throw empty_stack();
        }
        value = data.top();
        data.pop();
    }

    bool empty() const
    {
        std::lock_guard lock(mutex);
        return data.empty();
    }
};


#endif // THREADING_THREADSAFE_STACK_HPP