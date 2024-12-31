#include <cassert>
#include <print>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/ipc/system_v/semaphore_set.hpp"
#include "unix/utility.hpp"
#include "unix/signal.hpp"

int main(int, char **)
{
    using namespace unix;

    constexpr std::size_t process_to_create_count = 10;
    std::size_t child_processes_count{0};
    constexpr int child_sleep_duration{10};

    constexpr std::size_t sem_count = 1;
    constexpr int sem_index = 0;
    constexpr auto permissions = unix::permissions_builder{}
                                     .owner_can_read()
                                     .owner_can_write()
                                     .group_can_read()
                                     .group_can_write()
                                     .others_can_read()
                                     .others_can_write()
                                     .get();
    const auto semaphore_created = ipc::system_v::semaphore_set::create_private(sem_count, permissions);

    if (!semaphore_created)
    {
        std::println("failed to create a semmaphore set due to: {}",
                     unix::to_string(semaphore_created.error()).data());
        return EXIT_FAILURE;
    }
    auto &semaphore = semaphore_created.value();
    semaphore.set_value(int{0}, int{0});
    process_id_t last_created_process_id;

    for (std::size_t index{0}; index < process_to_create_count; ++index)
    {
        const auto process_created = unix::create_process();

        if (!process_created)
        {
            std::print("failed to create process due to: {}", unix::to_string(process_created.error()).data());
            std::abort();
        }
        const auto process_id = process_created.value();

        if (unix::is_child_process(process_id))
        {
            std::println("current process id: {}", unix::get_process_id());
            std::println("current process parent id: {}", unix::get_parent_process_id());

            std::println("ready for processing");
            assert(semaphore.increase_value(sem_index, 1));

            std::println("started processing...");
            sleep(child_sleep_duration);
            return EXIT_SUCCESS;
        }
        else
        {
            std::println("child process with id: {} created", process_id);
            std::println("index: {}", index);
            child_processes_count++;
            last_created_process_id = process_id;
        }
    }
    std::println("child processes count: {}", child_processes_count);
    assert(child_processes_count == process_to_create_count);

    std::println("wait for all chidlren to get ready for processing...");
    assert(semaphore.decrease_value(sem_index, -child_processes_count));

    std::println("requesting termination of the last created child process");
    unix::request_process_termination(last_created_process_id);

    std::println("wait for all children to finish processing...");
    bool all_finished{false};
    unix::process_id_t child_id;
    int status;

    while (true)
    {
        const auto child_terminated = unix::wait_till_child_terminates(&status);

        if (!child_terminated)
        {
            const auto error = child_terminated.error();

            if (error.code != ECHILD)
            {
                std::println("failed waiting for a child: {}", unix::to_string(error).data());
                return EXIT_FAILURE;
            }
            std::println("all children terminated");
            break;
        }
        const auto child_id = child_terminated.value();

        std::println("child with process id: {} terminated", child_id);

        if (WIFEXITED(status))
        {
            std::println("child exit status: {}", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            psignal(WTERMSIG(status), "child exit signal");
        }
    }
    std::println("all done");

    const auto semaphore_removed = semaphore.remove();

    if (!semaphore_removed)
    {
        std::println("failed to remove semaphore due to: {}", unix::to_string(semaphore_removed.error()).data());
        return EXIT_FAILURE;
    }
    std::println("semaphore successfully removed");
    return EXIT_SUCCESS;
}