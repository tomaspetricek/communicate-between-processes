#ifndef POSIX_IPC_PIPE_HPP
#define POSIX_IPC_PIPE_HPP

#include <array>
#include <cassert>
#include <errno.h>
#include <expected>
#include <unistd.h>

#include "posix/error_code.hpp"
#include "posix/utility.hpp"
#include "posix/ipc/primitive.hpp"


namespace posix::ipc
{
    class pipe : public ipc::primitive
    {
        using file_descriptor_t = int;
        static constexpr std::size_t end_count = 2;
        static constexpr std::size_t read_end_index = 0;
        static constexpr std::size_t write_end_index = 1;
        using file_descriptors_t = std::array<file_descriptor_t, end_count>;

        std::expected<void, error_code> close_end(std::size_t index)
        {
            assert(index < end_count);
            assert(is_end_open(index));
            const auto ret = close(fds_[index]);

            if (operation_successful(ret))
            {
                end_open_ &= ~(1 << index); // set bit to 0
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
            ;
        }

        bool is_end_open(std::size_t index) const
        {
            assert(index < end_count);
            return end_open_ & (1 << index);
        }

    public:
        // find a way to make it private
        explicit pipe(const file_descriptors_t &fds) noexcept : fds_{fds} {}

        static std::expected<posix::ipc::pipe, error_code> create() noexcept
        {
            file_descriptor_t fds[end_count];
            const auto ret = ::pipe(fds);

            if (operation_successful(ret))
            {
                return std::expected<posix::ipc::pipe, error_code>{std::in_place, std::to_array(fds)};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        bool is_read_end_open() const
        {
            return is_end_open(read_end_index);
        }

        bool is_write_end_open() const
        {
            return is_end_open(write_end_index);
        }

        std::expected<void, error_code> write(void *data, size_t count) const noexcept
        {
            assert(is_write_end_open());
            const auto ret = ::write(fds_[write_end_index], data, count);

            if (!operation_failed(ret))
            {
                assert(ret == count);
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected(error_code{errno});
        }

        std::expected<void, error_code> read(void *data, size_t count) const noexcept
        {
            assert(is_read_end_open());
            const auto ret = ::read(fds_[read_end_index], data, count);

            if (!operation_failed(ret))
            {
                assert(ret <= count);
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};
        }

        std::expected<void, error_code> close_read_end() noexcept
        {
            return close_end(read_end_index);
        }

        std::expected<void, error_code> close_write_end() noexcept
        {
            return close_end(write_end_index);
        }

        ~pipe() noexcept
        {
            if (is_read_end_open())
            {
                close_read_end();
            }
            if (is_write_end_open())
            {
                close_write_end();
            }
        }

    private:
        file_descriptors_t fds_;
        uint8_t end_open_{0b11111111};
    };
} // namespace posix::ipc

#endif // POSIX_IPC_PIPE_HPP