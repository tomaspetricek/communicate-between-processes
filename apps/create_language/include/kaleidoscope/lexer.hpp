#ifndef KALEIDOSCOPE_LEXER_HPP
#define KALEIDOSCOPE_LEXER_HPP

#include <ctype.h>
#include <stdio.h>
#include <string>

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
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_LEXER_HPP