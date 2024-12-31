#ifndef UNIX_IPC_SYSTEM_V_SHARED_MEMORY_HPP
#define UNIX_IPC_SYSTEM_V_SHARED_MEMORY_HPP

#include <cassert>
#include <expected>
#include <sys/shm.h>
#include <memory>

#include "core/deleter.hpp"

#include "unix/error_code.hpp"
#include "unix/utility.hpp"
#include "unix/ipc/system_v/key.hpp"
#include "unix/ipc/system_v/primitive.hpp"

namespace unix::ipc::system_v
{
    static void detach_shared_memory(void *shm_ptr) noexcept
    {
        const auto ret = shmdt(shm_ptr);
        assert(!operation_failed(ret));
    }

    using shared_memory_ptr_t = std::unique_ptr<void, core::deleter<detach_shared_memory>>;

    class shared_memory : public primitive
    {
        using handle_type = int;

    public:
        explicit shared_memory(handle_type handle) noexcept
            : handle_{handle} {}

        static std::expected<shared_memory, error_code> create(system_v::key_t key, size_t size, int flags) noexcept
        {
            const auto handle = shmget(key, size, flags);

            if (operation_failed(handle))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<shared_memory, error_code>{std::in_place, handle};
        }

        static std::expected<shared_memory, error_code> open_existing(system_v::key_t key) noexcept
        {
            return create(key, int{0}, int{0});
        }

        static std::expected<shared_memory, error_code> create_if_absent(system_v::key_t key, size_t size, int permissions) noexcept
        {
            assert(key > system_v::key_t{0});
            return create(key, size, IPC_CREAT | permissions);
        }

        static std::expected<shared_memory, error_code> create_exclusively(system_v::key_t key, size_t size, int permissions) noexcept
        {
            assert(key > system_v::key_t{0});
            return create(key, size, IPC_CREAT | IPC_EXCL | permissions);
        }

        static std::expected<shared_memory, error_code> create_private(size_t size, int permissions) noexcept
        {
            return create(get_private_key(), size, permissions);
        }

        std::expected<shared_memory_ptr_t, error_code> attach(const void *attachment_address, int flags) const noexcept
        {
            auto *ret = shmat(handle_, attachment_address, flags);

            if (ret == (void *)-1)
            {
                return std::unexpected{error_code{errno}};
            }
            return shared_memory_ptr_t{ret};
        }

        std::expected<shared_memory_ptr_t, error_code> attach_anywhere(int flags) const noexcept
        {
            return attach(nullptr, flags);
        }

        std::expected<void, error_code> remove() const noexcept
        {
            const auto ret = shmctl(handle_, IPC_RMID, nullptr);

            if (operation_failed(ret))
            {
                return std::unexpected{error_code{errno}};
            }
            return std::expected<void, error_code>{};
        }

    private:
        handle_type handle_;
    };
}

#endif // UNIX_SYSTEM_IPC_V_SHARED_MEMORY_HPP