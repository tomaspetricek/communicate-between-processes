#ifndef POSIX_OPEN_FLAGS_HPP
#define POSIX_OPEN_FLAGS_HPP

#include <cassert>
#include <fcntl.h>

namespace posix
{
    enum class access_mode
    {
        read_only,  // O_RDONLY
        write_only, // O_WRONLY
        read_write, // O_RDWR
    };

    enum class creation_mode
    {
        already_exists,
        create,           // O_CREAT - if exists just open, otherwise create it
        exclusive_create, // O_CREAT | O_EXCL - create only if it does not exist
    };

    using open_flags_t = int;
    constexpr open_flags_t no_open_flags{0};

    static open_flags_t translate_open_flag(creation_mode mode) noexcept
    {
        if (mode == creation_mode::create)
        {
            return O_CREAT;
        }
        if (mode == creation_mode::exclusive_create)
        {
            return O_CREAT | O_EXCL;
        }
        assert(mode == creation_mode::already_exists);
        return no_open_flags;
    }

    static open_flags_t translate_open_flag(access_mode mode) noexcept
    {
        if (mode == access_mode::read_only)
        {
            return O_RDONLY;
        }
        if (mode == access_mode::write_only)
        {
            return O_WRONLY;
        }
        assert(mode == access_mode::read_write);
        return O_RDWR;
    }
}

#endif // POSIX_OPEN_FLAGS_HPP