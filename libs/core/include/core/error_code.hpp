#ifndef CORE_ERROR_CODE_HPP
#define CORE_ERROR_CODE_HPP

#include <array>
#include <cassert>
#include <string_view>

namespace core
{
    enum class error_code
    {
        again,
        full,
        empty,
        not_enough_space,
        count
    };

    std::string_view to_string(const error_code &error) noexcept
    {
        constexpr std::size_t count = static_cast<std::size_t>(error_code::count) + 1;
        assert(error < error_code::count);
        static const std::array<std::string_view, count> names{
            {{"again"}, {"full"}, {"empty"}, {"not_enough_space"}, {"count"}}};
        return names[static_cast<std::size_t>(error)];
    }
} // namespace core

#endif // CORE_ERROR_CODE_HPP