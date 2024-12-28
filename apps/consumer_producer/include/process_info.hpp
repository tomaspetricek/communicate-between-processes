#ifndef PROCESS_INFO_HPP
#define PROCESS_INFO_HPP

#include <cstddef>

namespace
{
    struct process_info
    {
        std::size_t created_process_count{0};
        bool is_child{false};
        bool is_producer{false};
        std::size_t group_id{0};
    };
} // namespace

#endif // PROCESS_INFO_HPP