#ifndef UNIX_IPC_POSIX_OPEN_FLAGS_BUILDER
#define UNIX_IPC_POSIX_OPEN_FLAGS_BUILDER

#include "unix/ipc/posix/open_flags.hpp"

namespace unix::ipc::posix
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

        constexpr concrete_builder_type &create_exclusively() noexcept
        {
            flags_ |= O_CREAT | O_EXCL;
            return *static_cast<concrete_builder_type*>(this);
        }

        constexpr open_flags_t get() const noexcept
        {
            return flags_;
        }
    };
} // namespace unix::ipc::posix

#endif // UNIX_IPC_POSIX_OPEN_FLAGS_BUILDER