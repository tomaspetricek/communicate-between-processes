#ifndef KALEIDOSCOPE_PARSER_HPP
#define KALEIDOSCOPE_PARSER_HPP

#include <ctype.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <unordered_map>
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

    // holds precedence for each binary operator that is defined
    static std::unordered_map<char, int> BinopPrecedence{
        {'<', 10},
        {'+', 20},
        {'-', 30},
        {'*', 40} // highest
    };

    // get the precedence of the pending binary operator token
    static int GetTokPrecendence() {
        if (!isascii(CurTok)) {
            return -1;
        }

        // make sure that it's declared in the map
        const int TokPrec = BinopPrecedence[CurTok];

        if (TokPrec <= 0) {
            return -1;
        }
        return TokPrec;
    }

    // binoprhs
    //  ::= ('+' primary)
    static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
        // if it is binop find it's precedence
        while (true) {
            int TokPrec = GetTokPrecendence();

            // if this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done
            if (TokPrec < ExprPrec) {
                return LHS;
            }

            // know that it is binop
            int BinOp = CurTok;
            getNextToken(); // eat binop

            // parse the primary expression after the binary operator
            auto RHS = ParsePrimary();

            if (!RHS) {
                return nullptr;
            }
            // if binop binds less tightly with rhs than the operator rhs, let
            // the pending operator take rhs as its lhs
            int NextPrec = GetTokPrecendence();

            if (TokPrec < NextPrec) {
                RHS = ParseBinOpRHS(TokPrec+1, std::move(RHS));

                if (!RHS) {
                    return nullptr;
                }
            }
            LHS = std::make_unique<BinarayExprAST>(BinOp, std::move(LHS), std::move(RHS));
        }
    }

    // expression
    //  ::= primary binoprhs
    static std::unique_ptr<ExprAST> ParseEpression() noexcept {
        auto lhs = ParsePrimary();

        if (!lhs) {
            return nullptr;
        }
        return ParseBinOpRHS(0, std::move(lhs));
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_PARSER_HPP