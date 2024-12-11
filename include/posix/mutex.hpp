#ifndef POSIX_MUTEX_HPP
#define POSIX_MUTEX_HPP

#include <cassert>
#include <errno.h>
#include <expected>
#include <pthread.h>

#include "error_code.hpp"
#include "posix/utility.hpp"

namespace posix
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

            if (operation_successful(ret))
            {
                return std::expected<posix::mutex, error_code>{std::in_place, handle};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        std::expected<void, error_code> lock() noexcept
        {
            const auto ret = pthread_mutex_lock(&handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        std::expected<void, error_code> unlock() noexcept
        {
            const auto ret = pthread_mutex_unlock(&handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        ~mutex() noexcept
        {
            const auto ret = pthread_mutex_destroy(&handle_);
            assert(operation_successful(ret));
        }

    private:
        pthread_mutex_t handle_;
    };
} // namespace posix

#endif // POSIX_MUTEX_HPP