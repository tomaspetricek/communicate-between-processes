#ifndef POSIX_IPC_SHARED_MEMORY_HPP
#define POSIX_IPC_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <string>
#include <sys/mman.h>

#include "posix/error_code.hpp"
#include "posix/ipc/utility.hpp"
#include "posix/ipc/primitive.hpp"


namespace posix::ipc
{
    class shared_memory : public ipc::primitive
    {
    public:
        static std::expected<shared_memory, error_code> create(std::string name) noexcept
        {
            assert(is_valid_pathname(name));
        }

        std::expected<void, error_code> unlink() noexcept {
            // shm_unlink
        }
    };
}

#endif // POSIX_IPC_SHARED_MEMORY_HPP