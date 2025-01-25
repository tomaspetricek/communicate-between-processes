#ifndef KALEIDOSCOPE_PARSER_HPP
#define KALEIDOSCOPE_PARSER_HPP

#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

#include "kaleidoscope/lexer.hpp"

// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
namespace kaleidoscope
{
    struct ExprAST
    {
        virtual ~ExprAST() = default;
    };

    // numeric literal
    class NumberExprAST : public ExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val) : Val{Val} {}
    };

    class VariableExprAST : public ExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name{Name} {}
    };

    // binary operator
    class BinarayExprAST : public ExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinarayExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                       std::unique_ptr<ExprAST> RHS)
            : Op{Op}, LHS(std::move(LHS)), RHS(std::move(LHS)) {}
    };

    // function calls
    class CallExprAST : public ExprAST
    {
        std::string Calle;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &Calle,
                    std::vector<std::unique_ptr<ExprAST>> Args)
            : Calle(Calle), Args(std::move(Args)) {}
    };

    // repersents prototype for a function
    class PrototypeAST
    {
        std::string Name;
        std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
            : Name{Name}, Args{std::move(Args)} {}

        const std::string &getName() const { return Name; }
    };

    // represents function definition itself
    class FunctionAST
    {
        std::unique_ptr<PrototypeAST> Proto;
        std::unique_ptr<ExprAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::unique_ptr<ExprAST> Body)
            : Proto{std::move(Proto)}, Body{std::move(Body)} {}
    };

    // provide simple token buffer
    static int CurTok; // is current token that the parser is looking at

    // updates current token
    static int getNextToken()
    {
        return CurTok = gettok();
    }

    // helpers for error handling
    std::unique_ptr<ExprAST> LogError(const char *Str)
    {
        fprintf(stderr, "Error: %s\n", Str);
        return nullptr;
    }

    // P - stands for prototype
    std::unique_ptr<PrototypeAST> LogErrorP(const char *Str)
    {
        LogError(Str);
        return nullptr;
    }

    // numberexpr ::= number
    static std::unique_ptr<ExprAST> ParseNumberExpr()
    {
        auto Result = std::make_unique<NumberExprAST>(NumVal);
        getNextToken(); // consume the number
        return std::move(Result);
    }

    // parentexpr ::= '(' expression ')'
    static std::unique_ptr<ExprAST> ParseParenExpr()
    {
        getNextToken();             // eat (
        auto V = ParseExpression(); // allows to handle recursive grammars

        if (!V)
        {
            return nullptr;
        }

        if (CurTok != ')')
        {
            return LogError("expected ')'");
        }
        getNextToken(); // eat )
        return V;       // once the parser constructs AST, parentheses are not needed
    }

    // identifierexpr
    //  ::= identifier
    //  ::= identifier '(' expression* ')'
    static std::unique_ptr<ExprAST> ParseIdentifierExpr()
    {
        std::string IdName = IdentifierStr;

        getNextToken(); // eat identifier

        if (CurTok != '(')
        { // simple variable reference // look ahead
            return std::make_unique<VariableExprAST>(IdName);
        }

        getNextToken(); // eat (
        std::vector<std::unique_ptr<ExprAST>> Args;

        if (CurTok != ')')
        {
            while (true)
            {
                if (auto Arg = ParseExpression())
                {
                    Args.push_back(std::move(Arg));
                }
                else
                {
                    return nullptr;
                }

                if (CurTok == ')')
                {
                    break;
                }

                if (CurTok != ',')
                {
                    return LogError("Expected ')' or ',' in argument list");
                }
                getNextToken();
            }
        }
        getNextToken();
        return std::make_unique<CallExprAST>(IdName, std::move(Args));
    }

    // primary
    //  ::= identifierexpr
    //  ::= numberexpr
    //  ::= parenexpr
    static std::unique_ptr<ExprAST> ParsePrimary()
    {
        switch (CurTok)
        {
        default:
            return LogError("unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenExpr();
        }
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_PARSER_HPP