#ifndef UNIX_POSIX_FS_UTILITY_HPP
#define UNIX_POSIX_FS_UTILITY_HPP

#include <cassert>
#include <expected>
#include <string_view>
#include <sys/stat.h>

#include "unix/error_code.hpp"
#include "unix/utility.hpp"

namespace unix::posix::fs
{
    std::expected<void, error_code> create_directory(std::string_view path, mode_t mode) noexcept
    {
        const auto ret = mkdir(path.data(), mode);

        if (operation_failed(ret))
        {
            return std::unexpected{error_code{errno}};
        }
        assert(operation_successful(ret));
        return std::expected<void, error_code>{};
    }
}

#endif // UNIX_POSIX_FS_UTILITY_HPP