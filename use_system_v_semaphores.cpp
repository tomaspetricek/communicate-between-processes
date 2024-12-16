#include <cassert>
#include <print>
#include <array>
#include <span>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/system_v/ipc/key.hpp"
#include "unix/system_v/ipc/open_flags_builder.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"

int main(int, char **)
{
    using namespace unix::system_v;

    constexpr std::size_t sem_count = 3;
    constexpr auto permissions = unix::permissions_builder{}
                                     .owner_can_read()
                                     .owner_can_write()
                                     .group_can_read()
                                     .group_can_write()
                                     .others_can_read()
                                     .others_can_write()
                                     .get();
    auto semaphores_created = ipc::semaphore_set::create_private(sem_count, permissions);

    if (!semaphores_created)
    {
        std::println("failed to create a semmaphore set due to: {}",
                     unix::to_string(semaphores_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("semaphore set created");
    auto &semaphores = semaphores_created.value();

    std::array<unsigned short, sem_count> init_values{{0, 1, 2}};
    const auto all_initialized = semaphores.set_values(init_values);

    if (!all_initialized)
    {
        std::println("failed to initialize semaphore set valued: {}", unix::to_string(all_initialized.error()).data());
        return EXIT_FAILURE;
    }
    std::println("all semaphores from the set initialized");

    std::array<unsigned short, sem_count> current_values;
    const auto retrieved_semaphore_values = semaphores.get_values(current_values);

    if (!retrieved_semaphore_values)
    {
        std::println("failed to retrive current semaphore values: {}", unix::to_string(retrieved_semaphore_values.error()).data());
        return EXIT_FAILURE;
    }
    std::print("current semaphore values are: ");

    for (const auto &curr_value : current_values)
    {
        std::print("{} ", curr_value);
    }
    std::println("");
    return EXIT_SUCCESS;
}