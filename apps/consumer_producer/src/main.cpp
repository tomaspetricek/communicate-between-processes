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

#include "message_queue.hpp"
#include "occupation.hpp"
#include "process_info.hpp"
#include "role.hpp"

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

process_info create_child_processes(std::size_t child_producer_count,
                                    std::size_t child_consumer_count) noexcept
{
    constexpr std::size_t process_counts_size{2};
    using process_counts_t = std::array<std::size_t, process_counts_size>;
    static_assert(process_counts_size == std::size_t{2});
    const auto process_counts =
        process_counts_t{child_producer_count, child_consumer_count};
    process_info info;
    info.is_child = true;
    std::size_t i{0};
    assert(info.is_producer == false);

    for (std::size_t c{0}; c < process_counts.size(); ++c)
    {
        info.is_producer = c;

        for (i = 0; i < process_counts[c]; ++i)
        {
            const auto process_created = unix::create_process();

            if (!process_created)
            {
                std::println("failed to create child process due to: {}",
                             unix::to_string(process_created.error()).data());
                break;
            }
            info.created_process_count++;

            if (!unix::is_child_process(process_created.value()))
            {
                std::println("created child process with id: {}",
                             process_created.value());
            }
            else
            {
                return info;
            }
        }
    }
    info.is_child = false;
    info.is_producer = true;
    info.group_id = i;
    return info;
}

struct shared_data
{
    std::atomic<bool> done_flag{false};
    std::atomic<std::int32_t> produced_message_count{0};
    std::atomic<std::int32_t> consumed_message_count{0};
    message_queue_t message_queue;
};

class processor
{
public:
    explicit processor(const social_role_t &role,
                       const occupation_t &occupation) noexcept
        : role_{role}, occupation_{occupation} {}

    bool process() noexcept
    {
        if (!std::visit([](const auto &r)
                        { return r.setup(); }, role_))
        {
            return false;
        }
        if (!std::visit([](auto &o)
                        { return o.run(); }, occupation_))
        {
            return false;
        }
        if (!std::visit([](auto &r)
                        { return r.finalize(); }, role_))
        {
            return false;
        }
        return true;
    }

private:
    social_role_t role_;
    occupation_t occupation_;
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
        0, 0, message_queue_t::capacity(), 0};
    const auto semaphore_initialized = semaphores.set_values(init_values);

    if (!semaphore_initialized)
    {
        std::println("failed to initialize semaphores");
        return EXIT_FAILURE;
    }
    std::println("semaphores initialized");
    const auto info =
        create_child_processes(child_producer_count, child_consumer_count);

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

    social_role_t role;

    if (info.is_child)
    {
        role = role::child{process_id, children_readiness_notifier};
    }
    else
    {
        role = role::parent{info,
                            children_readiness_notifier,
                            producers_notifier,
                            message_written_notifier,
                            data->consumed_message_count,
                            data->produced_message_count,
                            data->done_flag};
    }
    occupation_t occupation;

    if (info.is_producer)
    {
        occupation = occupation::producer{info,
                                          message_count,
                                          producers_notifier,
                                          message_read_notifier,
                                          message_written_notifier,
                                          data->message_queue,
                                          data->produced_message_count};
    }
    else
    {
        occupation = occupation::consumer{info,
                                          process_id,
                                          message_read_notifier,
                                          message_written_notifier,
                                          data->message_queue,
                                          data->done_flag,
                                          data->consumed_message_count};
    }
    if (!processor{role, occupation}.process())
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}