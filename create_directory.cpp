#include <print>
#include <string_view>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/posix/fs/utility.hpp"

int main(int, char **)
{
    constexpr std::string_view dir_path{
        "/Users/tomaspetricek/Documents/repos/communicate-between-processes/"
        "created-directory"};
    constexpr auto perms = unix::permissions_builder{}
                               .owner_can_read()
                               .owner_can_write()
                               .owner_can_execute()
                               .group_can_read()
                               .group_can_write()
                               .group_can_execute()
                               .others_can_read()
                               .others_can_write()
                               .others_can_execute()
                               .get();
    const auto directory_created =
        unix::posix::fs::create_directory(dir_path, perms);

    if (!directory_created)
    {
        std::println("failed to create a directory: {}",
                     unix::to_string(directory_created.error()).data());
    }
    else
    {
        std::println("directory created");
    }
    const auto file_info_retrieved = unix::posix::fs::get_file_info(dir_path);

    if (!file_info_retrieved)
    {
        std::println("failed to retrieve directory info: {}",
                     unix::to_string(file_info_retrieved.error()).data());
    }
    else
    {
        std::println("directory info retrieved");
        const auto &info = file_info_retrieved.value();
        assert(info.is_directory());
        std::println("size (bytes): {}", info.size());
        const auto created = info.created();
        const auto created_secs =
            static_cast<float>(created.tv_sec) +
            (static_cast<float>(created.tv_nsec) / 1'000'000'000.F);
        std::println("created: {}", created_secs);
        std::println("last modified: {}", info.last_modified());
        std::println("last accessed: {}", info.last_accessed());
    }
    return EXIT_SUCCESS;
}