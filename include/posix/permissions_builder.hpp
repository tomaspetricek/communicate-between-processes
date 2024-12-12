#ifndef POSIX_PERMISSIONS_HPP
#define POSIX_PERMISSIONS_HPP

#include <cassert>
#include <sys/stat.h>

namespace posix
{
    class permissions_builder
    {
        mode_t mode_{0};

    public:
        permissions_builder &owner_can_read() noexcept
        {
            mode_ |= S_IRUSR;
            return *this;
        }

        permissions_builder &owner_can_write() noexcept
        {
            mode_ |= S_IWUSR;
            return *this;
        }

        permissions_builder &owner_can_execute() noexcept
        {
            mode_ |= S_IXUSR;
            return *this;
        }

        permissions_builder &group_can_read() noexcept
        {
            mode_ |= S_IRGRP;
            return *this;
        }

        permissions_builder &group_can_write() noexcept
        {
            mode_ |= S_IWGRP;
            return *this;
        }

        permissions_builder &group_can_execute() noexcept
        {
            mode_ |= S_IXGRP;
            return *this;
        }

        permissions_builder &others_can_read() noexcept
        {
            mode_ |= S_IROTH;
            return *this;
        }

        permissions_builder &others_can_write() noexcept
        {
            mode_ |= S_IWOTH;
            return *this;
        }

        permissions_builder &others_can_execute() noexcept
        {
            mode_ |= S_IXOTH;
            return *this;
        }

        mode_t get() const noexcept
        {
            return mode_;
        }
    };
} // namespace posix

#endif // POSIX_PERMISSIONS_HPP