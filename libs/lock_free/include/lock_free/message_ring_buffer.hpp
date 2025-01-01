#ifndef LOCK_FREE_MESSAGE_RING_BUFFER_HPP
#define LOCK_FREE_MESSAGE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <span>
#include <vector>

namespace lock_free
{
    template <std::size_t Capacity>
    class message_ring_buffer
    {
        void write(std::size_t &write_idx, const char *bytes,
                   std::size_t count) noexcept
        {
            std::size_t tail_size = std::min(count, buffer_.size() - write_idx);
            std::memcpy(buffer_.data() + write_idx, bytes, tail_size);

            if (tail_size == count)
            {
                write_idx = (write_idx + tail_size) % buffer_.size();
                return;
            }
            assert(tail_size < count);
            write_idx = 0;
            const std::size_t head_size = count - tail_size;
            std::memcpy(buffer_.data() + write_idx, bytes + tail_size, head_size);
            write_idx = (head_size) % buffer_.size();
        }

        // ToDo: avoid making copies
        void read(std::size_t &read_idx, char *bytes, std::size_t count) noexcept
        {
            std::size_t tail_size = std::min(count, buffer_.size() - read_idx);
            std::memcpy(bytes, buffer_.data() + read_idx, tail_size);

            if (tail_size == count)
            {
                read_idx = (read_idx + tail_size) % buffer_.size();
                return;
            }
            assert(tail_size < count);
            read_idx = 0;
            const std::size_t head_size = count - tail_size;
            std::memcpy(bytes + tail_size, buffer_.data() + read_idx, head_size);
            read_idx = (head_size) % buffer_.size();
        }

    public:
        bool try_push(std::span<const char> message) noexcept
        {
            auto write_idx = write_idx_.load(std::memory_order_relaxed);
            const auto read_idx = read_idx_.load(std::memory_order_acquire);
            const auto required_mem = message.size() + sizeof(std::size_t);

            // check if is full
            if (write_idx + 1 == read_idx)
            {
                return false;
            }
            const auto available_mem =
                (read_idx <= write_idx) ? ((buffer_.size() - write_idx) + read_idx - 1)
                                        : (read_idx - write_idx - 1);

            // check if enough space
            if (available_mem < required_mem)
            {
                return false;
            }
            const auto next_write_idx = (write_idx + required_mem) % buffer_.size();
            const bool success = write_idx_.compare_exchange_strong(
                write_idx, next_write_idx, std::memory_order_release,
                std::memory_order_relaxed);

            if (!success)
            {
                return false;
            }
            const auto message_size = message.size();
            write(write_idx, reinterpret_cast<const char *>(&message_size),
                  sizeof(std::size_t));
            write(write_idx, message.data(), message.size());
            return true;
        }

        bool try_pop(std::vector<char> &message)
        {
            auto read_idx = read_idx_.load(std::memory_order_relaxed);

            // check if is empty
            if (read_idx == write_idx_.load(std::memory_order_acquire))
            {
                return false;
            }
            std::size_t size;
            auto msg_read_idx = read_idx;
            read(msg_read_idx, reinterpret_cast<char *>(&size), sizeof(std::size_t));

            // allocator may throw an exception
            message.resize(size);
            const auto total_size = sizeof(std::size_t) + size;
            const auto next_read_idx = (read_idx + total_size) % buffer_.size();

            const bool success = read_idx_.compare_exchange_strong(
                read_idx, next_read_idx, std::memory_order_release,
                std::memory_order_relaxed);

            if (!success)
            {
                return false;
            }
            read(msg_read_idx, message.data(), size);
            return true;
        }

        bool empty() const
        {
            return read_idx_.load(std::memory_order_acquire) ==
                   write_idx_.load(std::memory_order_acquire);
        }

        bool full() const
        {
            return (write_idx_.load(std::memory_order_acquire) + 1) % buffer_.size() ==
                   read_idx_.load(std::memory_order_acquire);
        }

        static constexpr std::size_t capacity() noexcept { return Capacity; }

    private:
        std::array<char, Capacity> buffer_{};
        alignas(64) std::atomic<std::size_t> read_idx_{0},
            write_idx_{0}; // aligned to cache line
        static_assert(Capacity > 1, "capacity must be greather than one");
    };
} // namespace lock_free

#endif // LOCK_FREE_MESSAGE_RING_BUFFER_HPP