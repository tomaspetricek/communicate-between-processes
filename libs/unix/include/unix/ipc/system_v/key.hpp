#ifndef UNIX_IPC_SYSTEM_V_KEY_HPP
#define UNIX_IPC_SYSTEM_V_KEY_HPP

#include <cassert>
#include <errno.h>
#include <expected>
#include <filesystem>
#include <string_view>
#include <sys/ipc.h>

#include "unix/utility.hpp"
#include "unix/error_code.hpp"

namespace unix::ipc::system_v
{
    using key_t = ::key_t;

    std::expected<key_t, error_code> generate_key(const std::string_view &pathname, int proj_id) noexcept
    {
        // ToDo: use sys call directly
        assert(std::filesystem::exists(pathname.data()));
        assert(proj_id != int{0});
        const auto ret = ftok(pathname.data(), proj_id);

        if (!unix::operation_failed(ret))
        {
            return std::expected<key_t, error_code>{ret};
        }
        return std::unexpected{error_code{errno}};
    }

    constexpr key_t get_private_key() noexcept
    {
        return IPC_PRIVATE;
    }
}

#endif // UNIX_IPC_SYSTEM_V_KEY_HPP