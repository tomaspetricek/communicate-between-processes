#include <print>

#include "unix/permissions_builder.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"
#include "unix/error_code.hpp"

int main(int, char **)
{
    constexpr std::size_t mem_size{2084};
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
    auto shared_memory_created =
        unix::system_v::ipc::shared_memory::create_private(mem_size, perms);

    if (!shared_memory_created)
    {
        std::println("failed to create shared memory due to: {}", unix::to_string(shared_memory_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("shared memory created");
    auto &shared_memory = shared_memory_created.value();
    auto memory_attached = shared_memory.attach_anywhere(0);

    if (!memory_attached)
    {
        std::println("failed to attach shared memory due to: {}", unix::to_string(memory_attached.error()).data());
        return EXIT_FAILURE;
    }
    auto &memory = memory_attached.value();
    std::println("attached to shared memory");

    // write a message
    const char *message = "Hello, shared memory!";
    strcpy(static_cast<char *>(memory.get()), message);

    const auto shared_memory_marked_for_removal = shared_memory.remove();

    if (!shared_memory_marked_for_removal)
    {
        std::println("failed to mark shared memory for removal: {}", unix::to_string(shared_memory_marked_for_removal.error()).data());
        return EXIT_FAILURE;
    }
    std::println("shared memory marked for removal");
    return EXIT_SUCCESS;
}