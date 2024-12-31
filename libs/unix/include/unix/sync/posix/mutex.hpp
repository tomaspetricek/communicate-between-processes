#ifndef UNIX_SYNC_POSIX_MUTEX_HPP
#define UNIX_SYNC_POSIX_MUTEX_HPP

#include <cassert>
#include <errno.h>
#include <expected>
#include <pthread.h>

#include "unix/error_code.hpp"
#include "unix/utility.hpp"

namespace unix::sync::posix
{
    class mutex
    {
    public:
        // find a way to make it private
        explicit mutex(const pthread_mutex_t &handle) noexcept : handle_{handle} {}

        static std::expected<posix::mutex, error_code> create() noexcept
        {
            pthread_mutex_t handle;
            const auto ret = pthread_mutex_init(&handle, nullptr);

            // in pthread the return value is zero on success
            // on failure it is set directly to the error code, errno is not set
            if (unix::operation_successful(ret))
            {
                return std::expected<posix::mutex, error_code>{std::in_place, handle};
            }
            assert(!unix::operation_successful(ret));
            return std::unexpected{error_code{ret}};;
        }

        std::expected<void, error_code> lock() noexcept
        {
            const auto ret = pthread_mutex_lock(&handle_);

            if (unix::operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(!unix::operation_successful(ret));
            return std::unexpected{error_code{ret}};
        }

        std::expected<void, error_code> unlock() noexcept
        {
            const auto ret = pthread_mutex_unlock(&handle_);

            if (unix::operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(!unix::operation_successful(ret));
            return std::unexpected{error_code{ret}};
        }

        ~mutex() noexcept
        {
            const auto ret = pthread_mutex_destroy(&handle_);
            assert(unix::operation_successful(ret));
        }

    private:
        pthread_mutex_t handle_;
    };
} // namespace unix::sync::posix

#endif // UNIX_SYNC_POSIX_MUTEX_HPP