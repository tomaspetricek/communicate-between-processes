#ifndef KALEIDOSCOPE_TOKEN_HPP
#define KALEIDOSCOPE_TOKEN_HPP

#include <string>
#include <variant>

namespace kaleidoscope {
    struct eof_token
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
    using token_t = std::variant<eof_token, def_token, extern_token,
                                 identifier_token, number_token>;
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_TOKEN_HPP