#ifndef BARBER_PROCESS_INFO_HPP
#define BARBER_PROCESS_INFO_HPP

#include <cstddef>

namespace barber
{
    struct process_info
    {
        bool is_child{false};
        bool is_barber{false};
        std::size_t created_process_count{0};
        std::size_t group_id{0};
    };
} // namespace barber

#endif // BARBER_PROCESS_INFO_HPP