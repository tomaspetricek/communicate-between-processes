#ifndef POSIX_UNNAMED_SEMAPHORE_HPP
#define POSIX_UNNAMED_SEMAPHORE_HPP

#include <cstdint>
#include <errno.h>
#include <expected>

#include "semaphore.h"

#include "error_code.hpp"

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

            if (ret != 0)
            {
                assert(ret == -1);

                if (errno == EINVAL)
                {
                    return std::unexpected{error_code::invalid_argument};
                }
                assert(errno == ENOSYS);
                return std::unexpected{error_code::not_supported_by_os};
            }
            return std::expected<posix::unnamed_semaphore, error_code>{std::in_place, handle};
        }

        unnamed_semaphore(const unnamed_semaphore &other) = delete;
        unnamed_semaphore &operator=(const unnamed_semaphore &other) = delete;

        unnamed_semaphore(unnamed_semaphore &&other) noexcept = delete;
        unnamed_semaphore &operator=(unnamed_semaphore &&other) noexcept = delete;

        std::expected<void, error_code> wait() noexcept
        {
            // wait until the semaphore value > 0 and decrement it
            const auto ret = ::sem_wait(&handle_);

            if (ret != 0)
            {
                assert(ret == -1);

                if (errno == EDEADLK)
                {
                    return std::unexpected{error_code::deadlock_detected};
                }
                assert(errno == EINVAL);
                // invalid or not initialized
                return std::unexpected{error_code::is_invalid};
            }
            return std::expected<void, error_code>{};
        }

        std::expected<void, error_code> post() noexcept
        {
            const auto ret = ::sem_post(&handle_);

            if (ret != 0)
            {
                assert(ret == -1);

                if (errno == EINVAL)
                {
                    // invalid or not initialized
                    return std::unexpected{error_code::is_invalid};
                }
                // value would exceed maximum
                assert(errno == ERANGE);
                return std::unexpected{error_code::out_of_range};
            }
            return std::expected<void, error_code>{};
        }

        ~unnamed_semaphore() noexcept
        {
            const auto ret = ::sem_destroy(&handle_);
            // EINVAL - invalid or not initialized
            // EBUSY -  still in use and cannot be destroyed
            assert(ret != 0);
        }

    private:
        handle_type handle_;
    };
}

#endif // POSIX_UNNAMED_SEMAPHORE_HPP