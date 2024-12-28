#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <print>
#include <span>
#include <string_view>
#include <variant>

#include "lock_free/ring_buffer.hpp"
#include "string_literal.hpp"
#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"

#include "buffering/message_queue.hpp"
#include "buffering/occupation.hpp"
#include "buffering/process_info.hpp"
#include "buffering/processor.hpp"
#include "buffering/process_creation.hpp"
#include "buffering/role.hpp"

template <string_literal ResourceName, class Resource>
static void remove_resource(Resource *resource) noexcept
{
    const auto removed = resource->remove();
    std::println("{} removed: {}", ResourceName.data(), removed.has_value());
    assert(removed);
}

template <string_literal ResourceName, class Resource>
static void destroy_resource(Resource *resource) noexcept
{
    resource->~Resource();
    std::println("{} destroyed", ResourceName.data());
}

template <string_literal ResourceName, class Resource>
using resource_remover_t =
    std::unique_ptr<Resource,
                    unix::deleter<remove_resource<ResourceName, Resource>>>;

template <string_literal ResourceName, class Resource>
using resource_destroyer_t =
    std::unique_ptr<Resource,
                    unix::deleter<destroy_resource<ResourceName, Resource>>>;

struct shared_data
{
    std::atomic<bool> done_flag{false};
    std::atomic<std::int32_t> produced_message_count{0};
    std::atomic<std::int32_t> consumed_message_count{0};
    buffering::message_queue_t message_queue;
};

int main(int, char **)
{
    using namespace unix::system_v;

    constexpr std::size_t semaphore_count{4}, readiness_sem_index{0},
        written_message_sem_index{1}, read_message_sem_index{2},
        producer_sem_index{3};
    constexpr std::size_t mem_size{sizeof(shared_data)}, create_process_count{10},
        message_count{200}, child_producer_count{2}, child_consumer_count{10},
        producer_count{child_producer_count + 1},
        consumer_count{child_consumer_count},
        children_count{child_producer_count + child_consumer_count};
    constexpr std::size_t producer_frequenecy{5};
    static_assert(create_process_count >= producer_frequenecy);

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
    resource_remover_t<string_literal{"shared memory"}, ipc::shared_memory>
        shared_memory_remover{&shared_memory};

    auto semaphore_created =
        ipc::semaphore_set::create_private(semaphore_count, perms);

    if (!semaphore_created)
    {
        std::println("failed to create semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores created");
    auto &semaphores = semaphore_created.value();
    resource_remover_t<string_literal{"semaphore set"}, ipc::semaphore_set>
        semaphore_remover{&semaphores};

    const auto message_written_notifier = unix::system_v::ipc::group_notifier{
        semaphores, written_message_sem_index, consumer_count};
    const auto message_read_notifier = unix::system_v::ipc::group_notifier{
        semaphores, read_message_sem_index, producer_count};
    const auto children_readiness_notifier = unix::system_v::ipc::group_notifier{
        semaphores, readiness_sem_index, children_count};
    const auto producers_notifier = unix::system_v::ipc::group_notifier{
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
    const auto info =
        buffering::create_child_processes(child_producer_count, child_consumer_count);

    if (!info.is_child)
    {
        if (info.created_process_count != children_count)
        {
            std::println("failed to create all children process");
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

    auto *data = new (memory.get()) shared_data{};
    resource_destroyer_t<string_literal{"data"}, shared_data> data_destroyer{
        data};

    if (info.is_child)
    {
        data_destroyer.release();
    }
    const auto process_id = unix::get_process_id();

    buffering::role_t role;

    if (info.is_child)
    {
        role = buffering::role::child{process_id, children_readiness_notifier};
    }
    else
    {
        role = buffering::role::parent{info,
                                       children_readiness_notifier,
                                       producers_notifier,
                                       message_written_notifier,
                                       data->consumed_message_count,
                                       data->produced_message_count,
                                       data->done_flag};
    }
    buffering::occupation_t occupation;

    if (info.is_producer)
    {
        occupation = buffering::occupation::producer{info,
                                                     message_count,
                                                     producers_notifier,
                                                     message_read_notifier,
                                                     message_written_notifier,
                                                     data->message_queue,
                                                     data->produced_message_count};
    }
    else
    {
        occupation = buffering::occupation::consumer{info,
                                                     process_id,
                                                     message_read_notifier,
                                                     message_written_notifier,
                                                     data->message_queue,
                                                     data->done_flag,
                                                     data->consumed_message_count};
    }
    if (!buffering::processor{role, occupation}.process())
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}