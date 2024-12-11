#ifndef POSIX_UNNAMED_SEMAPHORE_HPP
#define POSIX_UNNAMED_SEMAPHORE_HPP

#include <cstdint>
#include <errno.h>
#include <expected>

#include "semaphore.h"

#include "posix/error_code.hpp"
#include "posix/utility.hpp"

namespace posix
{
    enum class shared_between : std::uint8_t
    {
        threads,
        processes,
    };

    class unnamed_semaphore
    {
        using handle_type = sem_t;

    public:
        // find a way to make it private
        explicit unnamed_semaphore(const handle_type &handle) noexcept : handle_{handle} {}

        static std::expected<posix::unnamed_semaphore, error_code> create(shared_between shared, unsigned int init_value) noexcept
        {
            handle_type handle;
            const auto ret = ::sem_init(&handle, static_cast<int>(shared), init_value);

            if (operation_successful(ret))
            {
                return std::expected<posix::unnamed_semaphore, error_code>{std::in_place, handle};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        unnamed_semaphore(const unnamed_semaphore &other) = delete;
        unnamed_semaphore &operator=(const unnamed_semaphore &other) = delete;

        unnamed_semaphore(unnamed_semaphore &&other) noexcept = delete;
        unnamed_semaphore &operator=(unnamed_semaphore &&other) noexcept = delete;

        std::expected<void, error_code> wait() noexcept
        {
            // wait until the semaphore value > 0 and decrement it
            const auto ret = ::sem_wait(&handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        std::expected<void, error_code> post() noexcept
        {
            const auto ret = ::sem_post(&handle_);

            if (operation_successful(ret))
            {
                return std::expected<void, error_code>{};
            }
            assert(operation_failed(ret));
            return std::unexpected{error_code{errno}};;
        }

        ~unnamed_semaphore() noexcept
        {
            const auto ret = ::sem_destroy(&handle_);
            // EINVAL - invalid or not initialized
            // EBUSY -  still in use and cannot be destroyed
            assert(operation_successful(ret));
        }

    private:
        handle_type handle_;
    };
}

#endif // POSIX_UNNAMED_SEMAPHORE_HPP