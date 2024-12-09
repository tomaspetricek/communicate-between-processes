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
        improperly_initialized,
        not_owned,
        not_supported_by_os,
        is_invalid,
        out_of_range,
    };
}

#endif // ERROR_CODE_HPP