#ifndef UNIX_SYSTEM_V_IPC_OPEN_FLAGS_BUILDER_HPP
#define UNIX_SYSTEM_V_IPC_OPEN_FLAGS_BUILDER_HPP

#include <cassert>
#include <sys/ipc.h>

namespace unix::system_v::ipc
{
    using open_flags_t = int;
    constexpr open_flags_t no_open_flags = 0;

    class open_flags_builder
    {
        open_flags_t flags_{no_open_flags};

    public:
        constexpr explicit open_flags_builder() noexcept = default;

        constexpr open_flags_builder &open_existing() noexcept
        {
            // default creation
            return *this;
        };

        constexpr open_flags_builder &create_if_absent() noexcept
        {
            flags_ |= IPC_CREAT;
            return *this;
        }

        constexpr open_flags_builder &create_exclusively() noexcept
        {
            flags_ |= IPC_CREAT | IPC_EXCL;
            return *this;
        }

        constexpr open_flags_t get() const noexcept
        {
            return flags_;
        }
    };
}

#endif // UNIX_SYSTEM_V_IPC_OPEN_FLAGS_BUILDER_HPP