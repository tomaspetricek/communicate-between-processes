#include <cassert>
#include <print>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/system_v/ipc/key.hpp"
#include "unix/system_v/ipc/open_flags_builder.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"

int main(int, char **)
{
    using namespace unix::system_v;

    constexpr std::size_t sem_count = 1;
    const auto key = ipc::get_private_key();
    constexpr auto open_flags =
        ipc::open_flags_builder{}.create_exclusively().get();
    constexpr auto permissions = unix::permissions_builder{}
                                     .owner_can_read()
                                     .owner_can_write()
                                     .group_can_read()
                                     .group_can_write()
                                     .others_can_read()
                                     .others_can_write()
                                     .get();
    auto semaphores_created = ipc::semaphore_set::create(key, sem_count, open_flags | permissions);

    if (!semaphores_created)
    {
        std::println("failed to create a semmaphore due to: {}",
                     unix::to_string(semaphores_created.error()).data());
    }
    std::println("semaphore created");
}