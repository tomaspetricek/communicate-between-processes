#ifndef ERROR_CODE_HPP
#define ERROR_CODE_HPP

namespace
{
    enum class error_code
    {
        creation,
        invalid_argument,
        insufficient_memory,
        deadlock_detected,
        not_owned,
        not_supported_by_os,
        is_invalid,
        out_of_range,
        process_max_open_file_limit_reached,
        system_max_open_file_limit_reached,
        would_block,
        not_accessible,
        write_beyond_file_max_size,
        end_closed,
        no_space,
        no_data_available,
        interrupted,
        connection_reset,
        io_failure,
        insufficient_permissions,
        already_exists,
        name_too_long,
        not_exists,
        overflow,
        bad_alloc,
    };
}

#endif // ERROR_CODE_HPP