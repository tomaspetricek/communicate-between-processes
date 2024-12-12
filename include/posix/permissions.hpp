#ifndef POSIX_PERMISSIONS_HPP
#define POSIX_PERMISSIONS_HPP

#include <cassert>
#include <sys/stat.h>

namespace posix
{
    struct permission
    {
        bool can_read{false};
        bool can_write{false};
        bool can_execute{false};
    };

    class permissions
    {
        permission owner_;
        permission group_;
        permission others_;

        // ToDo: create helper function
        mode_t translate_owner() const noexcept
        {
            mode_t mode{0};

            // ToDo: make branch free
            if (owner_.can_read)
            {
                mode |= S_IRUSR;
            }
            if (owner_.can_write)
            {
                mode |= S_IWUSR;
            }
            if (owner_.can_execute)
            {
                mode |= S_IXUSR;
            }
            return mode;
        }

        // ToDo: create helper function
        mode_t translate_group() const noexcept
        {
            mode_t mode{0};

            // ToDo: make branch free
            if (group_.can_read)
            {
                mode |= S_IRGRP;
            }
            if (group_.can_write)
            {
                mode |= S_IWGRP;
            }
            if (group_.can_execute)
            {
                mode |= S_IXGRP;
            }
            return mode;
        }

        // ToDo: create helper function
        mode_t translate_others() const noexcept
        {
            mode_t mode{0};

            // ToDo: make branch free
            if (others_.can_read)
            {
                mode |= S_IROTH;
            }
            if (others_.can_write)
            {
                mode |= S_IWOTH;
            }
            if (others_.can_execute)
            {
                mode |= S_IXOTH;
            }
            return mode;
        }

    public:
        permissions &owner_can_read() noexcept
        {
            owner_.can_read = true;
            return *this;
        }

        permissions &owner_can_write() noexcept
        {
            owner_.can_write = true;
            return *this;
        }

        permissions &owner_can_execute() noexcept
        {
            owner_.can_execute = true;
            return *this;
        }

        permissions &group_can_read() noexcept
        {
            group_.can_read = true;
            return *this;
        }

        permissions &group_can_write() noexcept
        {
            group_.can_write = true;
            return *this;
        }

        permissions &group_can_execute() noexcept
        {
            group_.can_execute = true;
            return *this;
        }

        permissions &others_can_read() noexcept
        {
            others_.can_read = true;
            return *this;
        }

        permissions &others_can_write() noexcept
        {
            others_.can_write = true;
            return *this;
        }

        permissions &others_can_execute() noexcept
        {
            others_.can_execute = true;
            return *this;
        }

        mode_t translate() const noexcept
        {
            mode_t mode{0};
            mode |= translate_owner();
            mode |= translate_group();
            mode |= translate_others();
            return mode;
        }
    };
} // namespace posix

#endif // POSIX_PERMISSIONS_HPP