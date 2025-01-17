#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <functional>
#include <print>
#include <span>
#include <variant>

#include "lock_free/ring_buffer.hpp"
#include "core/string_literal.hpp"
#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/ipc/system_v/group_notifier.hpp"
#include "unix/ipc/system_v/semaphore_set.hpp"
#include "unix/ipc/system_v/shared_memory.hpp"
#include "unix/resource_remover.hpp"
#include "unix/resource_destroyer.hpp"

#include "buffering/message_queue.hpp"
#include "buffering/occupation.hpp"
#include "buffering/process_creation.hpp"
#include "buffering/process_info.hpp"
#include "buffering/processor.hpp"
#include "buffering/role.hpp"
#include "buffering/shared_data.hpp"

int main(int, char **)
{
    using namespace unix::ipc;

    constexpr std::size_t semaphore_count{4}, readiness_sem_index{0},
        written_message_sem_index{1}, read_message_sem_index{2},
        producer_sem_index{3}, mem_size{sizeof(buffering::shared_data)},
        create_process_count{10}, message_count{200}, child_producer_count{2},
        child_consumer_count{10}, producer_count{child_producer_count + 1},
        consumer_count{child_consumer_count},
        children_count{child_producer_count + child_consumer_count};
    static_assert(create_process_count > std::size_t{1});
    static_assert(producer_count > std::size_t{0});
    static_assert(consumer_count > std::size_t{0});

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
        system_v::shared_memory::create_private(mem_size, perms);

    if (!shared_memory_created)
    {
        std::println("failed to create shared memory due to: {}",
                     unix::to_string(shared_memory_created.error()).data());
        return EXIT_FAILURE;
    }
    std::println("shared memory created");
    auto &shared_memory = shared_memory_created.value();
    unix::resource_remover_t<core::string_literal{"shared memory"},
                                  system_v::shared_memory>
        shared_memory_remover{&shared_memory};

    auto semaphore_created =
        system_v::semaphore_set::create_private(semaphore_count, perms);

    if (!semaphore_created)
    {
        std::println("failed to create semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores created");
    auto &semaphores = semaphore_created.value();
    unix::resource_remover_t<core::string_literal{"semaphore set"},
                                  system_v::semaphore_set>
        semaphore_remover{&semaphores};

    const auto message_written_notifier = unix::ipc::system_v::group_notifier{
        semaphores, written_message_sem_index, consumer_count};
    const auto message_read_notifier = unix::ipc::system_v::group_notifier{
        semaphores, read_message_sem_index, producer_count};
    const auto children_readiness_notifier = unix::ipc::system_v::group_notifier{
        semaphores, readiness_sem_index, children_count};
    const auto producers_notifier = unix::ipc::system_v::group_notifier{
        semaphores, producer_sem_index, producer_count};

    std::array<unsigned short, semaphore_count> init_values = {
        0, 0, buffering::message_queue_t::capacity(), 0};
    const auto semaphore_initialized = semaphores.set_values(init_values);

    if (!semaphore_initialized)
    {
        std::println("failed to initialize semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores initialized");
    const auto info = buffering::create_child_processes(child_producer_count,
                                                        child_consumer_count);

    if (!info.is_child)
    {
        if (info.created_process_count != children_count)
        {
            std::println("failed to create all children processes");
            return EXIT_FAILURE;
        }
        assert(consumer_count > 0 && producer_count > 0);
        assert(info.is_producer);
    }

    // the parent process shall do the clean-up
    if (info.is_child)
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

    auto *data = new (memory.get()) buffering::shared_data{};
    unix::resource_destroyer_t<core::string_literal{"data"},
                                    buffering::shared_data>
        data_destroyer{data};

    if (info.is_child)
    {
        data_destroyer.release();
    }
    const auto process_id = unix::get_process_id();

    auto processor = buffering::create_processor(
        info, process_id, message_count, message_written_notifier,
        message_read_notifier, children_readiness_notifier, producers_notifier,
        data->done_flag, data->produced_message_count,
        data->consumed_message_count, data->message_queue);

    if (!processor.process())
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}