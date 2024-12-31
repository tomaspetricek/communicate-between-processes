#ifndef UNIX_FS_POSIX_UTILITY_HPP
#define UNIX_FS_POSIX_UTILITY_HPP

#include <cassert>
#include <expected>
#include <string_view>
#include <sys/stat.h>

#include "unix/error_code.hpp"
#include "unix/utility.hpp"

namespace unix::fs::posix
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

    using file_info_t = struct stat;

    class file_info
    {
        struct stat stat_;

    public:
        explicit file_info(const struct stat &info) noexcept : stat_{info} {}

        dev_t owning_device_id() const noexcept
        {
            return stat_.st_dev;
        }

        ino_t inode_number() const noexcept
        {
            return stat_.st_ino;
        }

        mode_t protection() const noexcept
        {
            return stat_.st_mode;
        }

        nlink_t hard_links_count() const noexcept
        {
            return stat_.st_nlink;
        }

        uid_t owner_user_id() const noexcept
        {
            return stat_.st_uid;
        }

        gid_t owner_group_id() const noexcept
        {
            return stat_.st_gid;
        }

        // bytes
        off_t size() const noexcept
        {
            return stat_.st_size;
        }

        blksize_t block_size() const noexcept
        {
            return stat_.st_blksize;
        }

        blkcnt_t allocated_blocks_count() const noexcept
        {
            return stat_.st_blocks;
        }

        time_t last_accessed() const noexcept
        {
            return stat_.st_atime;
        }

        time_t last_modified() const noexcept
        {
            return stat_.st_mtime;
        }

        time_t status_last_changed() const noexcept
        {
            return stat_.st_ctime;
        }

        bool is_regular_file() const noexcept
        {
            return S_ISREG(stat_.st_mode);
        }

        bool is_directory() const noexcept
        {
            return S_ISDIR(stat_.st_mode);
        }

        bool is_character_device() const noexcept
        {
            return S_ISCHR(stat_.st_mode);
        }

        bool is_block_device() const noexcept
        {
            return S_ISBLK(stat_.st_mode);
        }

        bool is_named_pipe() const noexcept
        {
            return S_ISFIFO(stat_.st_mode);
        }

        bool is_symbolic_link() const noexcept
        {
            return S_ISLNK(stat_.st_mode);
        }

        bool is_socket() const noexcept
        {
            return S_ISSOCK(stat_.st_mode);
        }

#ifdef __APPLE__
        struct timespec created() const noexcept
        {
            return stat_.st_birthtimespec;
        }
#endif
    };

    std::expected<file_info, error_code> get_file_info(std::string_view path) noexcept
    {
        struct stat res;
        const auto ret = stat(path.data(), &res);

        if (operation_failed(ret))
        {
            return std::unexpected{error_code{errno}};
        }
        assert(operation_successful(ret));
        return std::expected<file_info, error_code>{file_info{res}};
    }
}

#endif // UNIX_FS_POSIX_UTILITY_HPP