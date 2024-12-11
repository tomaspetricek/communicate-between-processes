#ifndef POSIX_UTILITY_HPP
#define POSIX_UTILITY_HPP

#include <limits.h>
#include

namespace posix
{
    constexpr bool operation_successful(int ret)
    {
        return ret == 0;
    }

    constexpr bool operation_failed(int ret)
    {
        return ret == -1;
    }

    bool is_valid_ipc_name(const std::string_view &name) noexcept
    {
        return (name.size() + 1 <= PATH_MAX) && (name.size() > 0) && (name[0] == '/')
                // to ensure cross OS portability, it should not contain additional slashes
                && (std::find(name.begin() + 1, name.end(), '/') == name.end());
    }
}

#endif // POSIX_UTILITY_HPP
