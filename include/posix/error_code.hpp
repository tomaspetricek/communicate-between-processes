#ifndef POSIX_ERROR_CODE_HPP
#define POSIX_ERROR_CODE_HPP

#include "errno.h"
#include <expected>
#include <string_view>


namespace posix
{
    struct error_code {
        using value_type = decltype(EINVAL);

        value_type code;

        explicit operator value_type() const noexcept {
            return code;
        }
    };

    std::string_view to_string(error_code error) noexcept
    {
        return strerror(error.code);
    }
}

#endif // POSIX_ERROR_CODE_HPP