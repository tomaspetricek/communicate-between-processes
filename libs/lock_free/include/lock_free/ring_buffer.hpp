#ifndef LOCK_FREE_RING_BUFFER_HPP
#define LOCK_FREE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <cassert>

namespace lock_free
{
    template <class Value, std::size_t Capacity>
    class ring_buffer
    {
        std::array<Value, Capacity> data_;
        alignas(64) std::atomic<std::size_t> read_idx_{0}, write_idx_{0}; // aligned to cache line
        static_assert(Capacity > 1, "capacity must be greather than one");

    public:
        bool try_push(const Value &value)
        {
            auto write_idx = write_idx_.load(std::memory_order_relaxed);
            const auto next_write_idx = (write_idx + 1) % data_.size();

            // check if is full
            if (next_write_idx == read_idx_.load(std::memory_order_acquire))
            {
                return false;
            }
            const bool success = write_idx_.compare_exchange_strong(write_idx, next_write_idx, std::memory_order_release, std::memory_order_relaxed);

            if (!success)
            {
                return false;
            }
            data_[write_idx] = value;
            return true;
        }

        std::optional<Value> try_pop()
        {
            auto read_idx = read_idx_.load(std::memory_order_relaxed);

            // check if is empty
            if (read_idx == write_idx_.load(std::memory_order_acquire))
            {
                return std::nullopt; // Buffer is empty
            }
            const auto next_read_idx = (read_idx + 1) % data_.size();
            const bool success = read_idx_.compare_exchange_strong(read_idx, next_read_idx, std::memory_order_release, std::memory_order_relaxed);

            if (!success)
            {
                return std::nullopt;
            }
            return data_[read_idx];
        }

        void push(const Value &value) noexcept
        {
            auto write_idx = write_idx_.load(std::memory_order_relaxed);

            while (true)
            {
                const auto next_write_idx = (write_idx + 1) % data_.size();

                if (write_idx_.compare_exchange_strong(write_idx, next_write_idx, std::memory_order_release, std::memory_order_relaxed))
                {
                    break;
                }
            }
            data_[write_idx] = value;
        }

        Value pop() noexcept
        {
            auto read_idx = read_idx_.load(std::memory_order_relaxed);

            while (true)
            {
                const auto next_read_idx = (read_idx + 1) % data_.size();

                if (read_idx_.compare_exchange_strong(read_idx, next_read_idx, std::memory_order_release, std::memory_order_relaxed))
                {
                    break;
                }
            }
            return data_[read_idx];
        }

        bool empty() const
        {
            return read_idx_.load(std::memory_order_acquire) == write_idx_.load(std::memory_order_acquire);
        }

        bool full() const
        {
            return (write_idx_.load(std::memory_order_acquire) + 1) % data_.size() == read_idx_.load(std::memory_order_acquire);
        }

        static constexpr std::size_t capacity() noexcept
        {
            return Capacity;
        }
    };
}

#endif // LOCK_FREE_RING_BUFFER_HPP