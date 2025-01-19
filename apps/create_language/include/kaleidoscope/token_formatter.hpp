

#ifndef KALEIDOSCOPE_TOKEN_FORMATTER_HPP
#define KALEIDOSCOPE_TOKEN_FORMATTER_HPP

#include "kaleidoscope/token.hpp"

#include <print>

template <>
struct std::formatter<kaleidoscope::eof_token> : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::eof_token &, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", "eof");
    }
};

// commands
template <>
struct std::formatter<kaleidoscope::def_token> : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::def_token &, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", "def");
    }
};

template <>
struct std::formatter<kaleidoscope::extern_token>
    : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::extern_token &, FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", "extern");
    }
};

// primary
template <>
struct std::formatter<kaleidoscope::identifier_token>
    : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::identifier_token &token,
                FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", token.identifier);
    }
};

template <>
struct std::formatter<kaleidoscope::number_token>
    : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::number_token &token,
                FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", token.number);
    }
};

template <>
struct std::formatter<kaleidoscope::unknown_token>
    : std::formatter<std::string>
{
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    template <class FormatContext>
    auto format(const kaleidoscope::unknown_token &token,
                FormatContext &ctx) const
    {
        return std::format_to(ctx.out(), "{}", token.value);
    }
};

#endif // KALEIDOSCOPE_TOKEN_FORMATTER_HPP