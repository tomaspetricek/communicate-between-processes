#ifndef LOCK_FREE_RING_BUFFER_HPP
#define LOCK_FREE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <cassert>

namespace lock_free
{
    // src: https://rigtorp.se/ringbuffer/
    template <class Value, std::size_t Capacity>
    class ring_buffer
    {
        std::array<Value, Capacity> data_;
        alignas(64) std::atomic<std::size_t> read_idx_{0}, write_idx_{0}; // aligned to cache line
        static_assert(Capacity > 0, "capacity must be greather than zero");

    public:
        bool full() const
        {
            const auto write_idx = write_idx_.load(std::memory_order_relaxed);
            auto next_write_idx = (write_idx + 1) % data_.size();
            return (next_write_idx == read_idx_.load(std::memory_order_relaxed));
        }

        void push(Value &&val)
        {
            const auto write_idx = write_idx_.load(std::memory_order_relaxed);
            auto next_write_idx = (write_idx + 1) % data_.size();
            assert(next_write_idx < data_.size());
            data_[write_idx] = std::move(val);
            write_idx_.store(next_write_idx, std::memory_order_release);
        }

        bool empty() const
        {
            const auto read_idx = read_idx_.load(std::memory_order_relaxed);
            return (read_idx == write_idx_.load(std::memory_order_relaxed));
        }

        Value pop()
        {
            const auto read_idx = read_idx_.load(std::memory_order_relaxed);
            auto val = std::move(data_[read_idx]);
            auto next_read_idx = (read_idx + 1) % data_.size();
            assert(next_read_idx < data_.size());
            read_idx_.store(next_read_idx, std::memory_order_release);
            return val;
        }

        static constexpr std::size_t capacity() noexcept
        {
            return Capacity;
        }
    };
}

#endif // LOCK_FREE_RING_BUFFER_HPP