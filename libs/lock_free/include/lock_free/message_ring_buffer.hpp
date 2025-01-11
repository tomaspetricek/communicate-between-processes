#ifndef LOCK_FREE_MESSAGE_RING_BUFFER_HPP
#define LOCK_FREE_MESSAGE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <expected>
#include <span>
#include <type_traits>
#include <vector>

#include "core/error_code.hpp"

namespace lock_free
{
    using message_t =
        std::aligned_storage_t<sizeof(char), alignof(std::max_align_t)>;

    template <std::size_t Capacity>
    class message_ring_buffer
    {
        template <class Copy>
        void copy_data(std::size_t &buffer_idx, std::size_t count,
                       Copy copy) noexcept
        {
            const auto tail_size = std::min(count, buffer_.size() - buffer_idx);
            copy(buffer_idx, tail_size);

            if (tail_size < count)
            {
                buffer_idx = 0;
                const auto head_size = count - tail_size;
                copy(buffer_idx, head_size);
                buffer_idx = head_size;
            }
            else
            {
                buffer_idx = (buffer_idx + tail_size) % buffer_.size();
            }
        }

        void write(std::size_t &write_idx,
                   const std::span<const message_t> &bytes) noexcept
        {
            const std::byte *output = reinterpret_cast<const std::byte *>(bytes.data());
            copy_data(
                write_idx, bytes.size(),
                [&output, this](std::size_t buffer_idx, std::size_t size) -> void
                {
                    std::memcpy(buffer_.data() + buffer_idx, output, size);
                    output += size;
                });
        }

        void read(std::size_t &read_idx, const std::span<message_t> &bytes) noexcept
        {
            assert((read_idx < Capacity) && "read index >= capacity");
            std::byte *input = reinterpret_cast<std::byte *>(bytes.data());
            copy_data(read_idx, bytes.size(),
                      [&input, this](std::size_t buffer_idx, std::size_t size) -> void
                      {
                          std::memcpy(input, buffer_.data() + buffer_idx, size);
                          input += size;
                      });
        }

    public:
        static constexpr std::size_t
        required_message_storage(std::size_t message_size) noexcept
        {
            return sizeof(std::size_t) + message_size;
        }

        std::expected<void, core::error_code>
        try_push(std::span<const message_t> message) noexcept
        {
            auto write_idx = write_idx_.load(std::memory_order_relaxed);
            const auto read_idx = read_idx_.load(std::memory_order_acquire);
            const auto required_mem = message.size() + sizeof(std::size_t);

            // check if is full
            if (write_idx + 1 == read_idx)
            {
                return std::unexpected{core::error_code::full};
            }
            const auto available_mem =
                (read_idx <= write_idx) ? ((buffer_.size() - write_idx) + read_idx - 1)
                                        : (read_idx - write_idx - 1);

            // check if enough space
            if (available_mem < required_mem)
            {
                return std::unexpected{core::error_code::not_enough_space};
            }
            const auto next_write_idx = (write_idx + required_mem) % buffer_.size();

            if (!write_idx_.compare_exchange_strong(write_idx, next_write_idx,
                                                    std::memory_order_release,
                                                    std::memory_order_relaxed))
            {
                return std::unexpected{core::error_code::again};
            }
            const auto message_size = message.size();
            write(write_idx, std::span<const message_t>{
                                 reinterpret_cast<const message_t *>(&message_size),
                                 sizeof(std::size_t)});
            write(write_idx, message);
            assert((write_idx == next_write_idx));
            return std::expected<void, core::error_code>{};
        }

        std::expected<void, core::error_code>
        try_pop(std::vector<message_t> &message)
        {
            auto read_idx = read_idx_.load(std::memory_order_relaxed);

            // check if is empty
            if (read_idx == write_idx_.load(std::memory_order_acquire))
            {
                return std::unexpected{core::error_code::empty};
            }
            std::size_t size{0};
            std::size_t msg_read_idx = read_idx;
            read(msg_read_idx,
                 std::span<message_t>{reinterpret_cast<message_t *>(&size),
                                      sizeof(std::size_t)});

            if (size > Capacity)
            {
                return std::unexpected{core::error_code::again};
            }
            const auto total_size = sizeof(std::size_t) + size;
            const auto next_read_idx = (read_idx + total_size) % buffer_.size();

            if (!read_idx_.compare_exchange_strong(read_idx, next_read_idx,
                                                   std::memory_order_release,
                                                   std::memory_order_relaxed))
            {
                return std::unexpected{core::error_code::again};
            }
            message.resize(size);
            read(msg_read_idx, std::span<message_t>{message.data(), size});
            return std::expected<void, core::error_code>{};
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
        std::array<message_t, Capacity> buffer_ = {};
        alignas(64) std::atomic<std::size_t> read_idx_{0}, write_idx_{0};
        static_assert(Capacity > 1, "capacity must be greather than one");
    };
} // namespace lock_free

#endif // LOCK_FREE_MESSAGE_RING_BUFFER_HPP