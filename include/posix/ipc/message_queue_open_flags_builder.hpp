#ifndef POSIX_IPC_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER
#define POSIX_IPC_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER

#include <array>
#include <cassert>

#include "posix/ipc/open_flags.hpp"
#include "posix/ipc/open_flags_builder.hpp"

namespace posix::ipc
{
    enum class access_mode
    {
        read_only,  // O_RDONLY
        write_only, // O_WRONLY
        read_write, // O_RDWR
    };

    class message_queue_open_flags_builder : public open_flags_builder<message_queue_open_flags_builder>
    {
    public:
        constexpr explicit message_queue_open_flags_builder(access_mode access) noexcept
        {
            constexpr std::size_t access_mode_count = 3;
            const auto access_index = static_cast<std::size_t>(access);
            assert(access_index >= std::size_t{0});
            assert(access_index < access_mode_count);
            const auto access_flags = std::array<open_flags_t, access_mode_count>{
                O_RDONLY, O_WRONLY, O_RDWR};
            flags_ |= access_flags[access_index];
        }

        constexpr message_queue_open_flags_builder &is_blocking() noexcept
        {
            // default mode
            return *this;
        }

        constexpr message_queue_open_flags_builder &is_non_blocking() noexcept
        {
            flags_ |= O_NONBLOCK;
            return *this;
        }
    };
} // namespace posix::ipc

#endif // POSIX_IPC_MESSAGE_QUEUE_OPEN_FLAGS_BUILDER