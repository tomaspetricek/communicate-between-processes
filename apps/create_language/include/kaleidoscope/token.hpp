#ifndef KALEIDOSCOPE_TOKEN_HPP
#define KALEIDOSCOPE_TOKEN_HPP

#include <string>
#include <variant>

namespace kaleidoscope
{
    struct eof_token
    {
    };
    struct left_parenthesis_token
    {
    };
    struct right_parenthesis_token
    {
    };
    struct comma_token
    {
    };

    // commands
    struct def_token
    {
    };
    struct extern_token
    {
    };

    // primary
    struct identifier_token
    {
        std::string identifier;
    };
    struct number_token
    {
        double number{0};
    };

    struct unknown_token
    {
        char value;
    };
    struct less_token
    {
    };
    struct plus_token
    {
    };
    struct minus_token
    {
    };
    struct star_token
    {
    };

    using token_t =
        std::variant<eof_token, left_parenthesis_token, right_parenthesis_token,
                     comma_token, def_token, extern_token, identifier_token,
                     number_token, unknown_token, less_token, plus_token, minus_token, star_token>;
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_TOKEN_HPP