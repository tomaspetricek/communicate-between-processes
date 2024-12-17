#include <cassert>
#include <print>
#include <cstdlib>
#include <unistd.h>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/posix/process.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"

int main(int, char **)
{
    using namespace unix;

    constexpr std::size_t process_to_create_count = 10;
    std::size_t child_processes_count{0};
    constexpr int child_sleep_duration{5};
    constexpr std::size_t op_count{1};

    constexpr std::size_t sem_count = 1;
    constexpr auto permissions = unix::permissions_builder{}
                                     .owner_can_read()
                                     .owner_can_write()
                                     .group_can_read()
                                     .group_can_write()
                                     .others_can_read()
                                     .others_can_write()
                                     .get();
    auto semaphores_created = system_v::ipc::semaphore_set::create_private(sem_count, permissions);

    if (!semaphores_created)
    {
        std::println("failed to create a semmaphore set due to: {}",
                     unix::to_string(semaphores_created.error()).data());
        return EXIT_FAILURE;
    }
    auto &semaphores = semaphores_created.value();
    semaphores.set_value(int{0}, int{0});

    for (std::size_t index{0}; index < process_to_create_count; ++index)
    {
        const auto process_created = unix::posix::create_process();

        if (!process_created)
        {
            std::print("failed to create process due to: {}", unix::to_string(process_created.error()).data());
            std::abort();
        }
        const auto process_id = process_created.value();

        if (unix::posix::is_child_process(process_id))
        {
            std::println("current process id: {}", unix::posix::get_process_id());
            std::println("current process parent id: {}", unix::posix::get_parent_process_id());

            std::println("processing...");
            sleep(child_sleep_duration);

            sembuf sem_ops[op_count];
            sem_ops[0].sem_num = 0;
            sem_ops[0].sem_op = 1;
            sem_ops[0].sem_flg = 0;
            assert(semaphores.change_values(std::span<sembuf>{sem_ops, op_count}));
            return EXIT_SUCCESS;
        }
        else
        {
            std::println("child process with id: {} created", process_id);
            std::println("index: {}", index);
            child_processes_count++;
        }
    }
    std::println("child processes count: {}", child_processes_count);
    assert(child_processes_count == process_to_create_count);
    sembuf sem_ops[op_count];
    sem_ops[0].sem_num = 0;
    sem_ops[0].sem_op = -child_processes_count;
    sem_ops[0].sem_flg = 0;
    std::println("wait for all children to finish processing...");
    assert(semaphores.change_values(std::span<sembuf>{sem_ops, op_count}));
    std::println("all done");
    return EXIT_SUCCESS;
}