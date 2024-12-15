#ifndef UNIX_ERROR_CODE_HPP
#define UNIX_ERROR_CODE_HPP

#include "errno.h"
#include <expected>
#include <string_view>


namespace unix
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

#endif // UNIX_ERROR_CODE_HPP