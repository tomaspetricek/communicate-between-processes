#ifndef BUFFERING_PROCESS_INFO_HPP
#define BUFFERING_PROCESS_INFO_HPP

#include <cstddef>

namespace buffering
{
    struct process_info
    {
        std::size_t created_process_count{0};
        bool is_child{false};
        bool is_producer{false};
        std::size_t group_id{0};
    };
}

#endif // BUFFERING_PROCESS_INFO_HPP