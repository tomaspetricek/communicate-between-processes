#ifndef KALEIDOSCOPE_LEXER_HPP
#define KALEIDOSCOPE_LEXER_HPP

#include <ctype.h>
#include <optional>
#include <stdio.h>
#include <string>
#include <variant>

#include "kaleidoscope/token.hpp"

// src: https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html
namespace kaleidoscope
{
    class lexer
    {
        char last_{' '};

    public:
        template<class Reader>
        token_t get_token(Reader& reader) noexcept
        {
            // skip any whitespace
            while (isspace(last_))
            {
                last_ = reader.read();
            }
            if (isalpha(last_))
            {
                std::string indentifier;
                indentifier += last_;

                while (isalnum((last_ = reader.read())))
                {
                    indentifier += last_;
                }
                if (indentifier == "def")
                {
                    return def_token{};
                }
                if (indentifier == "extern")
                {
                    return extern_token{};
                }
                return identifier_token{std::move(indentifier)};
            }
            if (isdigit(last_) || last_ == '.')
            {
                std::string num;

                // need better error checking
                do
                {
                    num += last_;
                    last_ = reader.read();
                } while (isdigit(last_) || last_ == '.');

                // handle error
                const auto value = strtod(num.c_str(), 0);
                return number_token{value};
            }
            if (last_ == '#')
            {
                // comment until end of line
                do
                {
                    last_ = reader.read();
                } while (last_ != EOF && last_ != '\n' && last_ != '\r');

                if (last_ != EOF)
                {
                    return get_token(reader);
                }
            }
            // check for end of file. don't eat the EOF
            if (last_ == EOF)
            {
                return eof_token{};
            }
            // otheriwise just return the character as its ascii value
            unknown_token token{};
            token.value = last_;
            last_ = reader.read();
            return token;
        }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEXER_HPP