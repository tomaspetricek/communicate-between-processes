#ifndef THREADING_THREADSAFE_QUEUE_HPP
#define THREADING_THREADSAFE_QUEUE_HPP

#include <condition_variable>
#include <queue>
#include <memory>

template <class T>
class threadsafe_queue
{
    mutable std::mutex mutex;
    std::queue<T> queue;
    std::condition_variable data_pushed;

public:
    threadsafe_queue() = default;

    threadsafe_queue(const threadsafe_queue &other)
    {
        std::lock_guard lock(other.mutex);
        queue = other.queue;
    }

    threadsafe_queue &operator=(const threadsafe_queue &) = delete;

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(std::move(value));
        data_pushed.notify_one();
    }

    bool try_pop(T &value)
    {
        std::lock_guard lock(mutex);

        if (queue.empty())
        {
            return false;
        }
        value = queue.front();
        queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard lock(mutex);

        if (queue.empty())
        {
            return false;
        }
        const auto value = std::make_shared(queue.front());
        queue.pop();
        return value;
    }

    void wait_and_pop(T &value)
    {
        std::unique_lock lock(mutex);
        data_pushed.wait(lock, [this]()
                         { return !queue.empty(); });
        value = queue.front();
        queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock lock(mutex);
        data_pushed.wait(lock, [this]()
                         { return !queue.empty(); });
        const auto value = std::make_shared(queue.front());
        queue.pop();
        return value;
    }

    bool empty() const
    {
        std::lock_guard lock(mutex);
        return queue.empty();
    }
};

#endif // THREADING_THREADSAFE_QUEUE_HPP