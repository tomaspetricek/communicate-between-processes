#ifndef UNIX_IPC_POSIX_UTILITY_HPP
#define UNIX_IPC_POSIX_UTILITY_HPP

#include <algorithm>
#include <limits.h>
#include <string_view>


namespace unix::ipc::posix
{
    bool is_valid_pathname(const std::string_view &name) noexcept
    {
        return (name.size() + 1 <= PATH_MAX) && (name.size() > 0) && (name[0] == '/')
                // to ensure cross OS portability, it should not contain additional slashes
                && (std::find(name.begin() + 1, name.end(), '/') == name.end());
    }
}

#endif // UNIX_IPC_POSIX_UTILITY_HPP