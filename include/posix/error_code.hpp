#ifndef ERROR_CODE_HPP
#define ERROR_CODE_HPP

#include "errno.h"
#include <expected>
#include <string_view>


namespace posix
{
    using error_code = decltype(EINVAL);

    std::string_view error_to_string(error_code error) noexcept
    {
        return strerror(error);
    }
}

#endif // ERROR_CODE_HPP