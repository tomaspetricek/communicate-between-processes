#include <string_view>
#include <print>

#include "unix/posix/fs/utility.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/error_code.hpp"

int main(int, char **)
{
    constexpr std::string_view dir_path{"/Users/tomaspetricek/Documents/repos/communicate-between-processes/created-directory"};
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
    const auto directory_created = unix::posix::fs::create_directory(dir_path, perms);

    if (!directory_created)
    {
        std::println("failed to create a directory: {}", unix::to_string(directory_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("directory created");
    return EXIT_SUCCESS;
}