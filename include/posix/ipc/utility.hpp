#ifndef POSIX_IPC_UTILITY_HPP
#define POSIX_IPC_UTILITY_HPP

#include <algorithm>
#include <limits.h>
#include <string_view>


namespace posix
{
    bool is_valid_ipc_name(const std::string_view &name) noexcept
    {
        return (name.size() + 1 <= PATH_MAX) && (name.size() > 0) && (name[0] == '/')
                // to ensure cross OS portability, it should not contain additional slashes
                && (std::find(name.begin() + 1, name.end(), '/') == name.end());
    }
}

#endif // POSIX_IPC_UTILITY_HPP