#ifndef CORE_VARIANT_CAST_HPP
#define CORE_VARIANT_CAST_HPP

#include <utility>
#include <variant>

namespace core
{
    // src: https://stackoverflow.com/questions/47203255/convert-stdvariant-to-another-stdvariant-with-super-set-of-types
    namespace impl
    {
        template <class... Args>
        struct variant_cast_proxy_lref
        {
            std::variant<Args...> &v;

            template <class... ToArgs>
            constexpr operator std::variant<ToArgs...>() &&
            {
                return std::visit(
                    [](auto &&arg) -> std::variant<ToArgs...>
                    {
                        return std::forward<decltype(arg)>(arg);
                    },
                    v);
            }
        };

        template <class... Args>
        struct variant_cast_proxy_constlref
        {
            const std::variant<Args...> &v;

            template <class... ToArgs>
            constexpr operator std::variant<ToArgs...>() &&
            {
                return std::visit(
                    [](auto &&arg) -> std::variant<ToArgs...>
                    {
                        return std::forward<decltype(arg)>(arg);
                    },
                    v);
            }
        };

        template <class... Args>
        struct variant_cast_proxy_rref
        {
            std::variant<Args...> &&v;

            template <class... ToArgs>
            constexpr operator std::variant<ToArgs...>() &&
            {
                return std::visit(
                    [](auto &&arg) -> std::variant<ToArgs...>
                    {
                        return std::forward<decltype(arg)>(arg);
                    },
                    std::move(v));
            }
        };
    } // namespace impl

    template <class... Args>
    constexpr impl::variant_cast_proxy_lref<Args...>
    variant_cast(std::variant<Args...> &v)
    {
        return {v};
    }

    template <class... Args>
    constexpr impl::variant_cast_proxy_constlref<Args...>
    variant_cast(const std::variant<Args...> &v)
    {
        return {v};
    }

    template <class... Args>
    constexpr impl::variant_cast_proxy_rref<Args...>
    variant_cast(std::variant<Args...> &&v)
    {
        return {std::move(v)};
    }
} // namespace core

#endif // CORE_STRING_LITERAL_HPP