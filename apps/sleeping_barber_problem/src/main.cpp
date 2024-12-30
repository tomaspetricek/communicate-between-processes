#include <chrono>
#include <cstdint>
#include <optional>
#include <print>
#include <thread>

#include "common/process.hpp"

#include "core/random_integer_generator.hpp"
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

#include "barber/occupation/barber.hpp"
#include "barber/occupation/customer_generator.hpp"
#include "barber/role/parent.hpp"
#include "barber/shared_data.hpp"
#include "barber/customer_queue.hpp"

int main(int, char **)
{
    std::println("sleeping barber problem");

    constexpr std::size_t barber_count{1}, customer_generator_count{5},
        children_count{barber_count + customer_generator_count}, sem_count{3},
        child_readiness_sem_index{0}, empty_chair_sem_index{1},
        customer_waiting_sem_index{2}, mem_size{sizeof(barber::shared_data)},
        min_haircut_duration{100}, max_haircut_duration{3 * min_haircut_duration},
        min_customer_arrival_duration{200},
        max_customer_arrival_duration{min_customer_arrival_duration * 10};
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
        {0, barber::waiting_chair_count, 0}};
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
        barber::role::create_child_processes(barber_count, customer_generator_count);

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

    auto *data = new (memory.get()) barber::shared_data{};
    unix::resource_destroyer_t<core::string_literal{"data"}, barber::shared_data>
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
            core::random_integer_generator<std::size_t> get_haircut_duration{
                min_haircut_duration, max_haircut_duration};
            std::println("started serving customers");

            if (!barber::occupation::serve_customer(
                    customer_waiting_notifier, empty_chair_notifier,
                    data->customer_queue, data->shop_closed, data->served_customers,
                    get_haircut_duration))
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            core::random_integer_generator<std::size_t> get_customer_arrival_duration{
                min_customer_arrival_duration, max_customer_arrival_duration};
            std::println("started generating customers");

            if (!barber::occupation::generate_customers(
                    customer_waiting_notifier, empty_chair_notifier,
                    data->customer_queue, data->next_customer_id, data->shop_closed,
                    data->refused_customers, get_customer_arrival_duration))
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