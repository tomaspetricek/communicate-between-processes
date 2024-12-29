#include <chrono>
#include <optional>
#include <print>
#include <thread>

#include "core/string_literal.hpp"
#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/resource_destroyer.hpp"
#include "unix/resource_remover.hpp"
#include "unix/signal.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"

#include "common/process.hpp"

#include "lock_free/ring_buffer.hpp"

using customer_t = std::int32_t;
constexpr std::size_t waiting_chair_count = 5;
using customer_queue_t =
    lock_free::ring_buffer<customer_t, waiting_chair_count>;

constexpr std::chrono::milliseconds haircut_duration{1'000};

bool serve_customer(
    const unix::system_v::ipc::group_notifier &customer_waiting_notifier,
    const unix::system_v::ipc::group_notifier &empty_chair_notifier,
    customer_queue_t &customer_queue, const std::atomic<bool> &shop_closed,
    std::atomic<std::int32_t> &served_customers) noexcept
{
    while (true)
    {
        std::println("waiting for customer");
        const auto customer_waiting = customer_waiting_notifier.wait_for_one();

        if (shop_closed.load(std::memory_order_relaxed))
        {
            std::println("shop closed");
            break;
        }
        if (!customer_waiting)
        {
            std::println("failed to waiting for customer waiting due to: {}",
                         unix::to_string(customer_waiting.error()).data());
            return false;
        }
        const auto customer = customer_queue.pop();
        std::println("trimming customer: {} hair", customer);
        std::this_thread::sleep_for(haircut_duration);
        served_customers.fetch_add(1, std::memory_order_relaxed);
        const auto &empty_chair = empty_chair_notifier.notify_one();

        if (!empty_chair)
        {
            std::println("failed to notify about empty chair due to: {}",
                         unix::to_string(empty_chair.error()).data());
            return false;
        }
    }
    return true;
}

constexpr std::chrono::milliseconds arrival_duration{1'500};

bool generate_customers(
    const unix::system_v::ipc::group_notifier &customer_waiting_notifier,
    const unix::system_v::ipc::group_notifier &empty_chair_notifier,
    customer_queue_t &customer_queue, std::atomic<customer_t> &next_customer_id,
    const std::atomic<bool> &shop_closed,
    std::atomic<std::int32_t> &refused_customers) noexcept
{
    while (true)
    {
        std::this_thread::sleep_for(arrival_duration);
        const auto customer = next_customer_id++;
        const auto empty_chair = empty_chair_notifier.try_waiting_for_one();

        if (shop_closed.load(std::memory_order_relaxed))
        {
            std::println("shop closed");
            break;
        }
        if (!empty_chair)
        {
            if (empty_chair.error().code != EAGAIN)
            {
                std::println("failed trying to wait for an empty chair due to: {}",
                             unix::to_string(empty_chair.error()).data());
                return false;
            }
            std::println("all chairs occupied, refusing customer: {}", customer);
            refused_customers.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
            std::println("adding customer: {}", customer);
            customer_queue.push(customer);
            const auto customer_waiting = customer_waiting_notifier.notify_one();

            if (!customer_waiting)
            {
                std::println("failed to notify about customer waiting due to: {}",
                             unix::to_string(customer_waiting.error()).data());
                return false;
            }
        }
    }
    return true;
}

struct process_info
{
    bool is_child{false};
    bool is_barber{false};
    std::size_t created_process_count{0};
    std::size_t group_id{0};
};

// ToDo: avoid copying
process_info
create_child_processes(std::size_t barber_count,
                       std::size_t customer_generator_count) noexcept
{
    constexpr std::size_t process_counts_size{2};
    using process_counts_t = std::array<std::size_t, process_counts_size>;
    static_assert(process_counts_size == std::size_t{2});
    const auto process_counts =
        process_counts_t{customer_generator_count, barber_count};
    process_info info;
    info.is_child = true;
    std::size_t i{0};
    assert(info.is_barber == false);

    for (std::size_t c{0}; c < process_counts.size(); ++c)
    {
        info.is_barber = c;

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
    info.is_barber = false;
    info.group_id = 0;
    return info;
}

struct shared_data
{
    std::atomic<bool> shop_closed{false};
    std::atomic<customer_t> next_customer_id{0};
    std::atomic<std::int32_t> served_customers{0};
    std::atomic<std::int32_t> refused_customers{0};
    customer_queue_t customer_queue;
};

int main(int, char **)
{
    std::println("sleeping barber problem");

    constexpr std::size_t barber_count{1}, customer_generator_count{5},
        children_count{barber_count + customer_generator_count}, sem_count{3},
        child_readiness_sem_index{0}, empty_chair_sem_index{1},
        customer_waiting_sem_index{2}, mem_size{sizeof(shared_data)};
    constexpr std::chrono::milliseconds shop_open_duration{60'000};

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
    auto semaphores_created =
        unix::system_v::ipc::semaphore_set::create_private(sem_count, perms);

    if (!semaphores_created)
    {
        std::println("failed to create semaphores due to: {}",
                     unix::to_string(semaphores_created.error()).data());
        return EXIT_FAILURE;
    }
    auto &semaphores = semaphores_created.value();

    unix::resource_remover_t<core::string_literal{"semaphores"},
                             unix::system_v::ipc::semaphore_set>
        semaphores_remover{&semaphores};

    std::array<unsigned short, sem_count> init_values{
        {0, waiting_chair_count, 0}};
    const auto all_initialized = semaphores.set_values(init_values);

    if (!all_initialized)
    {
        std::println("failed to initialize semaphore set valued: {}",
                     unix::to_string(all_initialized.error()).data());
        return EXIT_FAILURE;
    }
    std::println("all semaphores from the set initialized");

    const auto empty_chair_notifier = unix::system_v::ipc::group_notifier{
        semaphores, empty_chair_sem_index, customer_generator_count};
    const auto customer_waiting_notifier = unix::system_v::ipc::group_notifier{
        semaphores, customer_waiting_sem_index, barber_count};
    const auto children_readiness_notifier = unix::system_v::ipc::group_notifier{
        semaphores, child_readiness_sem_index, children_count};

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
    unix::resource_remover_t<core::string_literal{"shared memory"},
                             unix::system_v::ipc::shared_memory>
        shared_memory_remover{&shared_memory};

    const auto info =
        create_child_processes(barber_count, customer_generator_count);

    if (!info.is_child)
    {
        if (info.created_process_count != children_count)
        {
            std::println("failed to create all children processes");
            return EXIT_FAILURE;
        }
    }
    else
    {
        // release does not call deleter
        shared_memory_remover.release();
        semaphores_remover.release();
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
    unix::resource_destroyer_t<core::string_literal{"data"}, shared_data>
        data_destroyer{data};

    if (info.is_child)
    {
        if (!common::signal_readiness_to_parent(children_readiness_notifier))
        {
            return EXIT_FAILURE;
        }
    }
    else
    {
        if (!common::wait_till_all_children_ready(children_readiness_notifier))
        {
            return EXIT_FAILURE;
        }
    }

    if (info.is_child)
    {
        data_destroyer.release();
    }
    if (info.is_child)
    {
        if (info.is_barber)
        {
            std::println("started serving customers");

            if (!serve_customer(customer_waiting_notifier, empty_chair_notifier,
                                data->customer_queue, data->shop_closed,
                                data->served_customers))
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::println("started generating customers");

            if (!generate_customers(customer_waiting_notifier, empty_chair_notifier,
                                    data->customer_queue, data->next_customer_id,
                                    data->shop_closed, data->refused_customers))
            {
                return EXIT_FAILURE;
            }
        }
    }
    else
    {
        std::this_thread::sleep_for(shop_open_duration);
        std::println("barber shop closing");

        data->shop_closed.store(true);
        empty_chair_notifier.notify_all();
        customer_waiting_notifier.notify_all();
        std::println("waiting for all children to terminate");

        if (!common::wait_till_all_children_termninate())
        {
            return EXIT_FAILURE;
        }
        std::println("served customer count: {}",
                     data->served_customers.load(std::memory_order_relaxed));
        std::println("refused customers count: {}",
                     data->refused_customers.load(std::memory_order_relaxed));
        std::println("arrived customers count: {}",
                     data->next_customer_id.load(std::memory_order_relaxed));
        std::println("all done");
    }
    return EXIT_SUCCESS;
}