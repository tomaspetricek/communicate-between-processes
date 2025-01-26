#ifndef KALEIDOSCOPE_LEXER_HPP
#define KALEIDOSCOPE_LEXER_HPP

#include <ctype.h>
#include <optional>
#include <stdio.h>
#include <string>
#include <variant>

#include "etl/flat_map.h"

#include "kaleidoscope/token.hpp"

// src: https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html
namespace kaleidoscope
{
    using symbol_pair_t = std::pair<char, token_t>;
    static const auto symbols = etl::make_flat_map<char, token_t>(
        symbol_pair_t{'(', left_parenthesis_token{}},
        symbol_pair_t{')', right_parenthesis_token{}},
        symbol_pair_t{',', comma_token{}});

    template <class Value>
    Value to_digit(char digit) noexcept
    {
        constexpr char first_digit{'0'};
        assert(digit >= first_digit);
        return static_cast<Value>(digit - first_digit);
    }

    template <class Reader>
    double lex_number(char &last, Reader &reader) noexcept
    {
        double num{0};
        constexpr double digit_count{10};

        if (isdigit(last))
        {
            num *= digit_count;
            const auto digit = to_digit<double>(last);
            assert(digit != double{0});
            num += digit;
            last = reader.read();
        }
        while (isdigit(last))
        {
            num *= digit_count;
            num += to_digit<double>(last);
            last = reader.read();
        }
        if (last != '.')
        {
            return num;
        }
        last = reader.read();
        double factor{1.};

        while (isdigit(last))
        {
            factor /= digit_count;
            num += factor * to_digit<double>(last);
            last = reader.read();
        }
        return num;
    }

    template <class Reader>
    class lexer
    {
        char last_{' '};
        token_t curr_token_;
        Reader &reader_;

    public:
        explicit lexer(Reader &reader) noexcept : reader_{reader} {}

        token_t get_token() noexcept
        {
            // skip any whitespace
            while (isspace(last_))
            {
                last_ = reader_.read();
            }
            const auto it = symbols.find(last_);

            if (it != symbols.cend())
            {
                last_ = reader_.read(); // eat symbol
                return it->second;
            }
            if (isalpha(last_))
            {
                std::string indentifier;
                indentifier += last_;

                while (isalnum((last_ = reader_.read())))
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
                const auto value = lex_number(last_, reader_);
                return number_token{value};
            }
            if (last_ == '#')
            {
                // comment until end of line
                do
                {
                    last_ = reader_.read();
                } while (last_ != EOF && last_ != '\n' && last_ != '\r');

                if (last_ != EOF)
                {
                    return get_token();
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
            last_ = reader_.read();
            return token;
        }

        token_t get_next_token() noexcept { return curr_token_ = get_token(); }

        const token_t &current_token() const noexcept { return curr_token_; }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEXER_HPP