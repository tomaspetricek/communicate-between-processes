#ifndef POSIX_MUTEX_HPP
#define POSIX_MUTEX_HPP

#include <cassert>
#include <errno.h>
#include <expected>
#include <pthread.h>

#include "error_code.hpp"

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

            if (ret == 0)
            {
                return std::expected<posix::mutex, error_code>{std::in_place, handle};
            }
            assert(ret == -1);

            if (errno == EINVAL)
            {
                // invalid mutex attributes
                return std::unexpected{error_code::invalid_argument};
            }
            assert(errno == ENOMEM);
            // insufficient memory to initialize the mutex
            return std::unexpected{error_code::insufficient_memory};
        }

        std::expected<void, error_code> lock() noexcept
        {
            const auto ret = pthread_mutex_lock(&handle_);

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);

            if (errno == EDEADLK)
            {
                // deadlock detected
                return std::unexpected{error_code::deadlock_detected};
            }
            assert(errno == EINVAL);
            // mutex is not properly initialized
            return std::unexpected{error_code::improperly_initialized};
        }

        std::expected<void, error_code> unlock() noexcept
        {
            const auto ret = pthread_mutex_unlock(&handle_);

            if (ret == 0)
            {
                return std::expected<void, error_code>{};
            }
            assert(ret == -1);

            if (errno == EPERM)
            {
                // current thread does not own the mutex
                return std::unexpected{error_code::not_owned};
            }
            assert(errno == EINVAL);
            // mutex not properly initialized
            return std::unexpected{error_code::improperly_initialized};
        }

        ~mutex() noexcept
        {
            const auto ret = pthread_mutex_destroy(&handle_);
            // ret != 0 -> mutex is not properly initialized or is currently locked
            assert(ret == 0);
        }

    private:
        pthread_mutex_t handle_;
    };
} // namespace posix

#endif // POSIX_MUTEX_HPP