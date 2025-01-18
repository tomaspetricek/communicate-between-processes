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
    enum Token
    {
        tok_eof = -1,

        // commands
        tok_def = -2,
        tok_extern = -3,

        // primary
        tok_identifier = -4,
        tok_number = -5
    };

    static std::string IdentifierStr; // filled in if tok_identifier
    static double NumVal;             // filled in if tok_number

    // returns the next token from standard input
    static int gettok()
    {
        static int LastChar = ' ';

        // skip any whitespace
        while (isspace(LastChar))
        {
            LastChar = getchar();
        }
        if (isalpha(LastChar))
        {
            IdentifierStr = LastChar;

            while (isalnum((LastChar = getchar())))
            {
                IdentifierStr += LastChar;
            }
            if (IdentifierStr == "def")
            {
                return tok_def;
            }
            if (IdentifierStr == "extern")
            {
                return tok_extern;
            }
            return tok_identifier;
        }
        if (isdigit(LastChar) || LastChar == '.')
        {
            std::string NumStr;

            // need better error checking
            do
            {
                NumStr += LastChar;
                LastChar = getchar();
            } while (isdigit(LastChar) || LastChar == '.');

            NumVal = strtod(NumStr.c_str(), 0);
            return tok_number;
        }
        if (LastChar == '#')
        {
            // comment until end of line

            do
            {
                LastChar = getchar();
            } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

            if (LastChar != EOF)
            {
                return gettok();
            }
        }
        // check for end of file. Don't eat the EOF
        if (LastChar == EOF)
        {
            return tok_eof;
        }
        // otheriwise just return the character as its ascii value
        int ThisChar = LastChar;
        LastChar = getchar();
        return ThisChar;
    }

    class lexer
    {
        char last_{' '};

    public:
        std::optional<token_t> get_token() noexcept
        {
            // skip any whitespace
            while (isspace(last_))
            {
                last_ = getchar();
            }
            if (isalpha(last_))
            {
                std::string indentifier;
                indentifier += last_;

                while (isalnum((last_ = getchar())))
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
                    last_ = getchar();
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
                    last_ = getchar();
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
            last_ = getchar();
            return std::nullopt;
        }
    };
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEXER_HPP