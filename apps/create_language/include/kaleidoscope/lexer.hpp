#ifndef KALEIDOSCOPE_LEXER_HPP
#define KALEIDOSCOPE_LEXER_HPP

#include <ctype.h>
#include <optional>
#include <stdio.h>
#include <string>
#include <variant>

#include "etl/flat_map.h"

#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/token.hpp"

// src: https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl01.html
namespace kaleidoscope
{
    using symbol_pair_t = std::pair<char, token_t>;
    static const auto symbols = etl::make_flat_map<char, token_t>(
        symbol_pair_t{'(', left_parenthesis_token{}},
        symbol_pair_t{')', right_parenthesis_token{}},
        symbol_pair_t{',', comma_token{}},
        symbol_pair_t{'<', less_token{}},
        symbol_pair_t{'+', plus_token{}},
        symbol_pair_t{'-', minus_token{}},
        symbol_pair_t{'*', star_token{}});

    template <class Value>
    Value to_digit(char digit) noexcept
    {
        constexpr char first_digit{'0'};
        assert(digit >= first_digit);
        return static_cast<Value>(digit - first_digit);
    }

    double lex_number(char &last, buffer_reader &reader) noexcept
    {
        double num{0};
        constexpr double digit_count{10};

        if (isdigit(last))
        {
            num *= digit_count;
            const auto digit = to_digit<double>(last);
            assert(digit != double{0});
            num += digit;
            reader.read(last);
        }
        while (!reader.empty() && isdigit(last))
        {
            num *= digit_count;
            num += to_digit<double>(last);
            reader.read(last);
        }
        if (last != '.')
        {
            return num;
        }
        reader.read(last);
        double factor{1.};

        while (!reader.empty() && isdigit(last))
        {
            factor /= digit_count;
            num += factor * to_digit<double>(last);
            reader.read(last);
        }
        return num;
    }

    class lexer
    {
        char last_{' '};
        token_t curr_token_;
        buffer_reader &reader_;

    public:
        explicit lexer(buffer_reader &reader) noexcept : reader_{reader} {}

        bool get_token(token_t &result) noexcept
        {
            if (reader_.empty())
            {
                return false;
            }
            // skip any whitespace
            while (isspace(last_))
            {
                if (!reader_.read(last_)){
                    return false;
                }
            }
            const auto it = symbols.find(last_);

            if (it != symbols.cend())
            {
                reader_.read(last_); // eat symbol
                result = it->second;
                return true;
            }
            if (isalpha(last_))
            {
                std::string indentifier;
                indentifier += last_;

                while (reader_.read(last_) && isalnum((last_)))
                {
                    indentifier += last_;
                }
                if (indentifier == "def")
                {
                    result = def_token{};
                    return true;
                }
                if (indentifier == "extern")
                {
                    result = extern_token{};
                    return true;
                }
                result = identifier_token{std::move(indentifier)};
                return true;
            }
            if (isdigit(last_) || last_ == '.')
            {
                const auto value = lex_number(last_, reader_);
                result = number_token{value};
                return true;
            }
            if (last_ == '#')
            {
                // comment until end of line
                while (reader_.read(last_) && last_ != EOF && last_ != '\n' &&
                       last_ != '\r')
                {
                }
                if (last_ != EOF)
                {
                    return get_token(result);
                }
            }
            // check for end of file. don't eat the EOF
            if (last_ == EOF)
            {
                result = eof_token{};
                return true;
            }
            // otheriwise just return the character as its ascii value
            result = unknown_token{last_};
            reader_.read(last_);
            return true;
        }

        token_t get_next_token() noexcept
        {
            if (!get_token(curr_token_)) {
                curr_token_ = eof_token{};
            }
            return curr_token_;
        }

        const token_t &current_token() const noexcept { return curr_token_; }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEXER_HPP