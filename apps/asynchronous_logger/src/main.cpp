#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <print>
#include <span>
#include <string_view>
#include <thread>
#include <utility>

#include "lock_free/message_ring_buffer.hpp"

using namespace std::literals::chrono_literals;

namespace async
{
    template <std::size_t MessageQueueCapacity>
    class writer_group
    {
        std::atomic<bool> stop_flag{false};
        lock_free::message_ring_buffer<MessageQueueCapacity> &message_queue_;

    public:
        explicit writer_group(lock_free::message_ring_buffer<MessageQueueCapacity>
                                  &message_queue) noexcept
            : message_queue_{message_queue} {}

        void start(std::size_t writer_id) noexcept
        {
            std::vector<char> message;

            while (!stop_flag.load(std::memory_order_acquire))
            {
                if (message_queue_.try_pop(message))
                {
                    std::println("id: {}, msg: {}", writer_id,
                                 std::string_view{message.data(), message.size()});
                }
                std::this_thread::sleep_for(100ms);
            }
        }

        void stop() noexcept { stop_flag.store(true, std::memory_order_release); }
    };

    template <class First, class... Rest>
    constexpr std::size_t size_of() noexcept
    {
        return (sizeof(First) + ... + sizeof(Rest));
    }

    template <class... Args>
    static void format_string(const char *, const Args &...args) {}

    template <std::size_t Capacity>
    class message_buffer
    {
        using data_type = char;
        std::size_t size_{0};
        std::array<data_type, Capacity> data_ = {};

    public:
        void append(std::span<const char> values) noexcept
        {
            assert((size_ + values.size()) <= Capacity);

            for (std::size_t index{0};
                 index < std::min(values.size(), data_.size() - size_); ++index)
            {
                data_[size_++] = values[index];
            }
        }

        std::size_t size() const noexcept { return size_; }

        std::span<const char> span() const noexcept
        {
            return std::span<const char>{data_.data(), size_};
        }
    };

    template <std::size_t WriterCount, std::size_t MessageQueueCapacity>
    class logger
    {
        using writers_type = writer_group<MessageQueueCapacity>;
        std::array<std::thread, WriterCount> threads_;
        lock_free::message_ring_buffer<MessageQueueCapacity> message_queue_;
        writers_type writers_;

    public:
        explicit logger() noexcept : writers_{message_queue_}
        {
            for (std::size_t index{0}; index < threads_.size(); ++index)
            {
                threads_[index] =
                    std::move(std::thread{&writers_type::start, &writers_, index});
            }
        }

        ~logger() noexcept
        {
            writers_.stop();

            for (auto &thread : threads_)
            {
                thread.join();
            }
        }

        template <std::size_t Size, class... Args>
        void log(const char (&format)[Size], const Args &...args) noexcept
        {
            constexpr std::size_t message_size = Size;
            message_buffer<message_size> message;
            message.append(format);

            while (!message_queue_.try_push(message.span()))
            {
            }
        }
    };
} // namespace async

int main(int, char **)
{
    constexpr std::size_t worker_count{4}, message_queue_capacity{1'024},
        message_count{100};
    async::logger<worker_count, message_queue_capacity> logger;

    for (std::size_t index{0}; index < message_count; ++index)
    {
        logger.log("hello {}", 42, 24.5);
        std::this_thread::sleep_for(50ms);
    }
}