#ifndef LOCK_FREE_MESSAGE_RING_BUFFER_HPP
#define LOCK_FREE_MESSAGE_RING_BUFFER_HPP

#include <array>
#include <atomic>
#include <span>
#include <vector>
#include <type_traits>

namespace lock_free
{
    template <std::size_t Capacity>
    class message_ring_buffer
    {
        template <class Copy>
        void copy_data(std::size_t &buffer_idx, std::size_t count, Copy copy) noexcept
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
                   const std::span<const char> &bytes) noexcept
        {
            const char *output{bytes.data()};
            copy_data(write_idx, bytes.size(),
                      [&output, this](std::size_t buffer_idx, std::size_t size) -> void
                      {
                          std::memcpy(buffer_.data() + buffer_idx, output, size);
                          output += size;
                      });
        }

        void read(std::size_t &read_idx, const std::span<char> &bytes) noexcept
        {
            char *input{bytes.data()};
            copy_data(read_idx, bytes.size(),
                      [&input, this](std::size_t buffer_idx, std::size_t size) -> void
                      {
                          std::memcpy(input, buffer_.data() + buffer_idx, size);
                          input += size;
                      });
        }

    public:
        static constexpr std::size_t required_message_storage(std::size_t message_size) noexcept
        {
            return sizeof(std::size_t) + message_size;
        }

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

            if (!write_idx_.compare_exchange_strong(
                    write_idx, next_write_idx, std::memory_order_release,
                    std::memory_order_relaxed))
            {
                return false;
            }
            const auto message_size = message.size();
            write(write_idx,
                  std::span<const char>{reinterpret_cast<const char *>(&message_size),
                                        sizeof(std::size_t)});
            write(write_idx, std::span<const char>{message.data(), message.size()});
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
            read(msg_read_idx,
                 std::span<char>{reinterpret_cast<char *>(&size), sizeof(std::size_t)});

            // allocator may throw an exception
            message.reserve(size);
            const auto total_size = sizeof(std::size_t) + size;
            const auto next_read_idx = (read_idx + total_size) % buffer_.size();

            if (!read_idx_.compare_exchange_strong(
                    read_idx, next_read_idx, std::memory_order_release,
                    std::memory_order_relaxed))
            {
                return false;
            }
            message.assign(size, '\0');
            read(msg_read_idx, std::span<char>{message.data(), size});
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
        using buffer_type =
            std::aligned_storage_t<sizeof(char), alignof(std::max_align_t)>;
        std::array<buffer_type, Capacity> buffer_ = {};
        alignas(64) std::atomic<std::size_t> read_idx_{0},
            write_idx_{0};
        static_assert(Capacity > 1, "capacity must be greather than one");
    };
} // namespace lock_free

#endif // LOCK_FREE_MESSAGE_RING_BUFFER_HPP