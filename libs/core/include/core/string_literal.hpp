#ifndef CORE_STRING_LITERAL_HPP
#define CORE_STRING_LITERAL_HPP

#include <algorithm>
#include <cstddef>


namespace core
{
    template <std::size_t Size>
    struct string_literal
    {
        constexpr string_literal(const char (&str)[Size])
        {
            std::copy_n(str, Size, value);
        }
        char value[Size];

        constexpr const char *data() const noexcept
        {
            return value;
        }
    };
}

#endif // CORE_STRING_LITERAL_HPP