#include <memory>
#include <print>

#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/signal.hpp"
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

// ToDo: remove global state
constexpr std::size_t semaphore_count{3}, readiness_sem_index{0},
    written_message_sem_index{1}, read_message_sem_index{2};

bool signal_readiness_to_parent(unix::system_v::ipc::semaphore_set &semaphores) noexcept
{
    const auto readiness_signaled =
        semaphores.increase_value(readiness_sem_index, 1);

    if (!readiness_signaled)
    {
        std::println("failed to send signal about readiness due to: {}",
                     unix::to_string(readiness_signaled.error()).data());
        return false;
    }
    return true;
}

bool process_messages(unix::system_v::ipc::semaphore_set &semaphores,
                      const unix::system_v::ipc::shared_memory_ptr_t &memory,
                      const unix::process_id_t &process_id) noexcept
{
    std::println("wait for message");
    const auto message_written =
        semaphores.decrease_value(written_message_sem_index, -1);

    if (!message_written)
    {
        std::println(
            "failed to receive signal about message being written due to: {}",
            unix::to_string(message_written.error()).data());
        return false;
    }
    const auto *message = static_cast<char *>(memory.get());
    std::println("received message: {} by child with id: {}", message,
                 process_id);
    const auto message_read =
        semaphores.increase_value(read_message_sem_index, 1);

    if (!message_read)
    {
        std::println("failed to send signal about message being read due to: {}",
                     unix::to_string(message_read.error()).data());
        return false;
    }
    return true;
}

bool wait_till_all_children_ready(
    const std::size_t created_process_count,
    unix::system_v::ipc::semaphore_set &semaphores) noexcept
{
    using namespace unix::system_v;

    const auto readiness_signaled = semaphores.decrease_value(
        readiness_sem_index,
        -static_cast<ipc::semaphore_value_t>(created_process_count));

    if (!readiness_signaled)
    {
        std::println("signal about child being ready not received due to: {}",
                     unix::to_string(readiness_signaled.error()).data());
        return false;
    }
    return true;
}

bool generate_messages(
    const std::size_t created_process_count,
    unix::system_v::ipc::semaphore_set &semaphores,
    unix::system_v::ipc::shared_memory_ptr_t &memory) noexcept
{
    for (std::size_t i{0}; i < created_process_count; ++i)
    {
        // write a message
        const char *message = "Hello, shared memory!";
        strcpy(static_cast<char *>(memory.get()), message);

        std::println("message written into shared memeory");
        const auto message_written =
            semaphores.increase_value(written_message_sem_index, 1);

        if (!message_written)
        {
            std::println(
                "failed to send signal about message being written due to: {}",
                unix::to_string(message_written.error()).data());
            return false;
        }
        const auto message_read =
            semaphores.decrease_value(read_message_sem_index, -1);

        if (!message_read)
        {
            std::println(
                "failed to receive signal about message being read due to: {}",
                unix::to_string(message_read.error()).data());
            return false;
        }
    }
    return true;
}

bool wait_till_all_children_termninate() noexcept
{
    int status;

    while (true)
    {
        const auto child_terminated = unix::wait_till_child_terminates(&status);

        if (!child_terminated)
        {
            const auto error = child_terminated.error();

            if (error.code != ECHILD)
            {
                std::println("failed waiting for a child: {}",
                             unix::to_string(error).data());
                return false;
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
    return true;
}

int main(int, char **)
{
    using namespace unix::system_v;

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
        ipc::shared_memory::create_private(mem_size, perms);

    if (!shared_memory_created)
    {
        std::println("failed to create shared memory due to: {}",
                     unix::to_string(shared_memory_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("shared memory created");
    auto &shared_memory = shared_memory_created.value();
    resource_remover_t<ipc::shared_memory> shared_memory_remover{&shared_memory};

    auto semaphore_created =
        ipc::semaphore_set::create_private(semaphore_count, perms);

    if (!semaphore_created)
    {
        std::println("failed to create semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores created");
    auto &semaphores = semaphore_created.value();
    resource_remover_t<ipc::semaphore_set> semaphore_remover{&semaphores};

    std::array<unsigned short, semaphore_count> init_values = {0};
    const auto semaphore_initialized = semaphores.set_values(init_values);

    if (!semaphore_initialized)
    {
        std::println("failed to initialize semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores initialized");

    constexpr std::size_t create_process_count{10};
    std::size_t created_process_count{0};
    bool is_child{false};

    for (std::size_t i{0}; i < create_process_count; ++i)
    {
        const auto process_created = unix::create_process();

        if (!process_created)
        {
            std::println("failed to create child process due to: {}",
                         unix::to_string(process_created.error()).data());
            continue;
        }
        if (!unix::is_child_process(process_created.value()))
        {
            std::println("created child process with id: {}",
                         process_created.value());
        }
        else
        {
            is_child = true;
            break;
        }
        created_process_count++;
    }
    if (!is_child && (created_process_count == 0))
    {
        std::println("failed to create any child process");
        return EXIT_FAILURE;
    }

    // the parent process shall do the clean-up
    if (is_child)
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

    if (is_child)
    {
        const auto process_id = unix::get_process_id();

        if (!signal_readiness_to_parent(semaphores))
        {
            return EXIT_FAILURE;
        }
        std::println("child process with id: {} is ready", process_id);

        if (!process_messages(semaphores, memory, process_id))
        {
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::println("wait till all children process are ready");

        if (!wait_till_all_children_ready(create_process_count, semaphores))
        {
            return EXIT_FAILURE;
        }
        std::println("generate messeages");

        if (!generate_messages(create_process_count, semaphores, memory))
        {
            return EXIT_FAILURE;
        }
        std::println("wait for all children to terminate");

        if (!wait_till_all_children_termninate())
        {
            return EXIT_FAILURE;
        }
        std::println("all done");
    }
    return EXIT_SUCCESS;
}