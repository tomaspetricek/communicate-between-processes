#ifndef POSIX_SHARED_MEMORY_HPP
#define POSIX_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <string>
#include <sys/mman.h>

#include "posix/error_code.hpp"
#include "posix/utility.hpp"


namespace posix
{
    class shared_memory
    {
    public:
        static std::expected<posix::shared_memory, error_code> create(std::string name) noexcept
        {
            assert(is_valid_ipc_name(name));
        }

        std::expected<void, error_code> unlink() noexcept {
            // shm_unlink
        }

        shared_memory(const shared_memory &other) = delete;
        shared_memory &operator=(const shared_memory &other) = delete;

        shared_memory(shared_memory &&other) noexcept = delete;
        shared_memory &operator=(shared_memory &&other) noexcept = delete;
    };
}

#endif // POSIX_SHARED_MEMORY_HPP