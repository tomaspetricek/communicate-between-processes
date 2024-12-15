#ifndef POSIX_IPC_OPEN_FLAGS_BUILDER
#define POSIX_IPC_OPEN_FLAGS_BUILDER

#include "posix/ipc/open_flags.hpp"

namespace posix::ipc
{
    template <class ConcreteBuilder>
    class open_flags_builder
    {
        using concrete_builder_type = ConcreteBuilder;

    protected:
        open_flags_t flags_{no_open_flags};

        constexpr explicit open_flags_builder() noexcept = default;

    public:
        constexpr concrete_builder_type &open_existing() noexcept
        {
            // default creation
            return *static_cast<concrete_builder_type*>(this);
        };

        constexpr concrete_builder_type &create_if_absent() noexcept
        {
            flags_ |= O_CREAT;
            return *static_cast<concrete_builder_type*>(this);
        }

        constexpr concrete_builder_type &create_only() noexcept
        {
            flags_ |= O_CREAT | O_EXCL;
            return *static_cast<concrete_builder_type*>(this);
        }

        constexpr open_flags_t get() const noexcept
        {
            return flags_;
        }
    };
} // namespace posix::ipc

#endif // POSIX_IPC_OPEN_FLAGS_BUILDER