#ifndef BUFFERING_PROCESS_CREATION_HPP
#define BUFFERING_PROCESS_CREATION_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <print>

#include "unix/error_code.hpp"
#include "unix/process.hpp"

#include "buffering/process_info.hpp"

namespace buffering
{
    buffering::process_info
    create_child_processes(std::size_t child_producer_count,
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
} // namespace buffering

#endif // BUFFERING_PROCESS_CREATION_HPP