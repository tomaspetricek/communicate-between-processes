#include <memory>
#include <print>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"

template <class Resource>
static void remove_resource(Resource *resource) noexcept
{
    const auto removed = resource->remove();
    std::println("resource removed: {}", removed.has_value());
    assert(removed);
}

template <class Resource>
using resource_remover_t =
    std::unique_ptr<Resource, unix::deleter<remove_resource<Resource>>>;

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
        std::println("failed to create shared memory due to: {}",
                     unix::to_string(shared_memory_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("shared memory created");
    auto &shared_memory = shared_memory_created.value();
    resource_remover_t<unix::system_v::ipc::shared_memory> shared_memory_remover{
        &shared_memory};

    constexpr std::size_t semaphore_count{2}, readiness_sem_index{0},
        message_sem_index{1};
    auto semaphore_created = unix::system_v::ipc::semaphore_set::create_private(
        semaphore_count, perms);

    if (!semaphore_created)
    {
        std::println("failed to create semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores created");
    auto &semaphores = semaphore_created.value();
    resource_remover_t<unix::system_v::ipc::semaphore_set> semaphore_remover{
        &semaphores};

    std::array<unsigned short, semaphore_count> init_values{{0, 0}};
    const auto semaphore_initialized = semaphores.set_values(init_values);

    if (!semaphore_initialized)
    {
        std::println("failed to initialize semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores initialized");

    const auto process_created = unix::create_process();

    if (!process_created)
    {
        std::println("failed to create child process due to: {}",
                     unix::to_string(process_created.error()).data());
        return EXIT_FAILURE;
    }

    // the parent process shall do the clean-up
    if (unix::is_child_process(process_created.value()))
    {
        // release does not call deleter
        shared_memory_remover.release();
        semaphore_remover.release();
    }
    auto memory_attached = shared_memory.attach_anywhere(0);

    if (!memory_attached)
    {
        std::println("failed to attach shared memory due to: {}",
                     unix::to_string(memory_attached.error()).data());
        return EXIT_FAILURE;
    }
    auto &memory = memory_attached.value();
    std::println("attached to shared memory");

    if (unix::is_child_process(process_created.value()))
    {
        const auto readiness_signaled =
            semaphores.increase_value(readiness_sem_index, 1);

        if (!readiness_signaled)
        {
            std::println("failed to send signal about readiness due to: {}",
                         unix::to_string(readiness_signaled.error()).data());
            return EXIT_FAILURE;
        }
        std::println("child process ready");

        std::println("wait for message");
        const auto message_written =
            semaphores.decrease_value(message_sem_index, -1);

        if (!message_written)
        {
            std::println(
                "failed to receive signal about message being written due to: {}",
                unix::to_string(message_written.error()).data());
            return EXIT_FAILURE;
        }
        const auto *message = static_cast<char *>(memory.get());
        std::println("received message: {}", message);
    }
    else
    {
        std::println("wait till child process is prepared");
        const auto readiness_signaled =
            semaphores.decrease_value(readiness_sem_index, -1);

        if (!readiness_signaled)
        {
            std::println("signal about child being ready not received due to: {}",
                         unix::to_string(readiness_signaled.error()).data());
            return EXIT_FAILURE;
        }
        // write a message
        const char *message = "Hello, shared memory!";
        strcpy(static_cast<char *>(memory.get()), message);
        std::println("message written into shared memeory");

        const auto message_written =
            semaphores.increase_value(message_sem_index, 1);

        if (!message_written)
        {
            std::println(
                "failed to send signal about message being written due to: {}",
                unix::to_string(message_written.error()).data());
            return EXIT_FAILURE;
        }
        std::println("wait for child to terminate");
        int status;
        const auto child_terminated = unix::wait_till_child_terminates(&status);

        if (!child_terminated)
        {
            const auto error = child_terminated.error();

            if (error.code != ECHILD)
            {
                std::println("failed waiting for a child: {}",
                             unix::to_string(error).data());
                return EXIT_FAILURE;
            }
        }
        std::println("child terminated");
    }
    return EXIT_SUCCESS;
}