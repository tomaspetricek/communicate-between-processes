#ifndef BARBER_ROLE_PARENT_HPP
#define BARBER_ROLE_PARENT_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <print>

#include "unix/error_code.hpp"
#include "unix/process.hpp"

#include "barber/process_info.hpp"

namespace barber::role {
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
}

#endif // BARBER_ROLE_PARENT_HPP