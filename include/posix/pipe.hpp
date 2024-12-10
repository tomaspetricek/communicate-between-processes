#ifndef POSIX_PIPE_HPP
#define POSIX_PIPE_HPP

#include <array>
#include <cassert>
#include <errno.h>
#include <expected>
#include <unistd.h>

#include "error_code.hpp"
#include "posix/utility.hpp"

namespace posix
{
    class pipe
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
            }
            assert(operation_failed(ret));

            if (errno == EBADF)
            {
                // file descriptor is invalid, possibly because it has already been closed or was never opened
                return std::unexpected{error_code::is_invalid};
            }
            if (errno == EINTR)
            {
                // operation was interrupted by a signal before it could complete
                return std::unexpected{error_code::interrupted};
            }
            assert(errno == EIO);
            // low-level I/O error occurred when flushing data
            return std::unexpected{error_code::io_failure};
        }

        bool is_end_open(std::size_t index) const
        {
            assert(index < end_count);
            return end_open_ & (1 << index);
        }

    public:
        // find a way to make it private
        explicit pipe(const file_descriptors_t &fds) noexcept : fds_{fds} {}

        static std::expected<posix::pipe, error_code> create() noexcept
        {
            file_descriptor_t fds[end_count];
            const auto ret = ::pipe(fds);

            if (operation_successful(ret))
            {
                return std::expected<posix::pipe, error_code>{std::in_place, std::to_array(fds)};
            }
            assert(operation_failed(ret));

            if (errno == EFAULT)
            {
                // array pased is outside of accessible address space
                return std::unexpected{error_code::invalid_argument};
            }
            if (errno == EMFILE)
            {
                // process has reached its limit for open file descriptors
                return std::unexpected{error_code::process_max_open_file_limit_reached};
            }
            assert(errno == ENFILE);
            // system-wide limit for open file descriptors has been reached
            return std::unexpected{error_code::system_max_open_file_limit_reached};
        }

        pipe(const pipe &other) = delete;
        pipe &operator=(const pipe &other) = delete;

        pipe(pipe &&other) noexcept = delete;
        pipe &operator=(pipe &&other) noexcept = delete;

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

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // file descriptor is in non-blocking mode, and the write operation would block
                return std::unexpected{error_code::would_block};
            }
            if (errno == EBADF)
            {
                // file descriptor is invalid or not open for writing
                return std::unexpected(error_code::is_invalid);
            }
            if (errno == EFAULT)
            {
                // buffer pointer points outside the accessible address space
                return std::unexpected{error_code::not_accessible};
            }
            if (errno == EFBIG)
            {
                // attempt was made to write beyond a file's maximum allowable size
                return std::unexpected{error_code::write_beyond_file_max_size};
            }
            if (errno == EINVAL)
            {
                // invalid argument was passed
                return std::unexpected{error_code::invalid_argument};
            }
            if (errno == EPIPE)
            {
                // write end of the pipe is closed, and the process receives a SIGPIPE signal
                return std::unexpected(error_code::end_closed);
            }
            assert(errno == ENOSPC);
            // device or resource has run out of space
            return std::unexpected(error_code::no_space);
        }

        std::expected<void, error_code> read(void *data, size_t count) const noexcept
        {
            assert(is_read_end_open());
            const auto ret = ::read(fds_[read_end_index], data, count);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));

            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // file descriptor is in non-blocking mode, and there is no data available
                return std::unexpected{error_code::no_data_available};
            }
            if (errno == EBADF)
            {
                // file descriptor is invalid or not open for reading
                return std::unexpected{error_code::is_invalid};
            }
            if (errno == EFAULT)
            {
                // buffer pointer points outside the accessible address space
                return std::unexpected{error_code::not_accessible};
            }
            if (errno == EINVAL)
            {
                // invalid argument was passed
                return std::unexpected{error_code::invalid_argument};
            }
            if (errno == EINTR)
            {
                // read call was interrupted by a signal before any data was read
                return std::unexpected{error_code::interrupted};
            }
            assert(errno == EINTR);
            // connection was reset by the peer (for sockets or pipes connected across processes)
            return std::unexpected{error_code::connection_reset};
        }

        void close_read_end() noexcept
        {
            close_end(read_end_index);
        }

        void close_write_end() noexcept
        {
            close_end(write_end_index);
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
} // namespace posix

#endif // POSIX_PIPE_HPP