#ifndef KALEIDOSCOPE_ORIGINAL_PARSER_HPP
#define KALEIDOSCOPE_ORIGINAL_PARSER_HPP

#include <ctype.h>
#include <map>
#include <memory>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "kaleidoscope/original/lexer.hpp"

// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
namespace kaleidoscope
{
    static std::unique_ptr<LLVMContext> TheContext;
    static std::unique_ptr<IRBuilder<>> Builder;
    static std::unique_ptr<Module> TheModule; // llvm construct to keep global variables and functions, owns memmory for all IR
    static std::map<std::string, Value*> NamedValues; // symbol table for the code

    std::unique_ptr<ExprAST> LogError(const char *Str);

    Value *LogErrorV(const char* Str) {
        LogError(Str);
        return nullptr;
    }

    struct ExprAST
    {
        virtual ~ExprAST() = default;
        virtual Value *codegen() = 0;
    };

    // numeric literal
    class NumberExprAST : public ExprAST
    {
        double Val;

    public:
        NumberExprAST(double Val) : Val{Val} {}

        Value *codegen() final {
            return ConstantFP::get(*TheContext, APFloat(Val)); // constant floating poinnt
        }
    };

    class VariableExprAST : public ExprAST
    {
        std::string Name;

    public:
        VariableExprAST(const std::string &Name) : Name{Name} {}

        Value *codegen() final {
            // look this variable up in the function
            Value *V = NamedValues[Name];

            if (!V) {
                LogErrorV("Unnown variable name");
            }
            return V;
        }
    };

    // binary operator
    class BinaryExprAST : public ExprAST
    {
        char Op;
        std::unique_ptr<ExprAST> LHS, RHS;

    public:
        BinaryExprAST(char Op, std::unique_ptr<ExprAST> LHS,
                       std::unique_ptr<ExprAST> RHS)
            : Op{Op}, LHS(std::move(LHS)), RHS(std::move(LHS)) {}

        Value *codegen() final {
            // code is recursively emitted for the left-hand
            // side of the expression, then the right-hand side
            // then we compute the result of the binary expression
            Value *L = LHS->codegen();
            Value *R = RHS->codegen();

            if (!L || !R) {
                return nullptr;
            }
            switch (Op) {
                case '+':
                    return Builder->CreateFAdd(L, R, "addtmp");
                case '-':
                    return Builder->CreateFSub(L, R, "subtmp");
                case '*':
                    return Builder->createFMul(L, R, "multmp");
                case '<':
                    L = Builder->CreateFCmpULT(L, R, "cmptmp"); // compare u... less then
                    // Convert bool 0/1 to double 0.0 or 1.0
                    return Builder->CreateUIToFP(L, Type::getDoubleTy(*TheContext), "booltmp");
                default:
                    return LogErrorV("invalid binary operator");
            }
        }
    };

    // function calls
    class CallExprAST : public ExprAST
    {
        std::string Callee;
        std::vector<std::unique_ptr<ExprAST>> Args;

    public:
        CallExprAST(const std::string &Callee,
                    std::vector<std::unique_ptr<ExprAST>> Args)
            : Callee(Callee), Args(std::move(Args)) {}

        Value *codegen() final {
            // Look up the name in the global module table
            Function* CalleeF = TheModule->getFunction(Callee); // function name lookup

            if (!Callee) {
                return LogErrorV("Unknown function referenced");
            }
            // argument mismatch error
            if (CalleF->arg_size() != Args.size()) {
                return LogErrorV("Incorrect number of arguments passed");
            }
            std::vector<Value*> ArgsV;

            for (unsigned i = 0, e = Args.size(); i != e, ++i) {
                ArgsV.emplace_back(Args[i]->codegen());

                if (!ArgsV.back()) {
                    return nullptr;
                }
            }
            return Builder->CreateCall(CalleeF, ArgsV, "calltmp"); // call instruction
        }
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
    static int getNextToken() { return CurTok = gettok(); }

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

    static std::unique_ptr<ExprAST> ParseExpression();

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
        {'<', 10}, {'+', 20}, {'-', 30}, {'*', 40} // highest
    };

    // get the precedence of the pending binary operator token
    static int GetTokPrecendence()
    {
        if (!isascii(CurTok))
        {
            return -1;
        }

        // make sure that it's declared in the map
        const int TokPrec = BinopPrecedence[CurTok];

        if (TokPrec <= 0)
        {
            return -1;
        }
        return TokPrec;
    }

    // binoprhs
    //  ::= ('+' primary)
    static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                                  std::unique_ptr<ExprAST> LHS)
    {
        // if it is binop find it's precedence
        while (true)
        {
            int TokPrec = GetTokPrecendence();

            // if this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done
            if (TokPrec < ExprPrec)
            {
                return LHS;
            }

            // know that it is binop
            int BinOp = CurTok;
            getNextToken(); // eat binop

            // parse the primary expression after the binary operator
            auto RHS = ParsePrimary();

            if (!RHS)
            {
                return nullptr;
            }
            // if binop binds less tightly with rhs than the operator rhs, let
            // the pending operator take rhs as its lhs
            int NextPrec = GetTokPrecendence();

            if (TokPrec < NextPrec)
            {
                RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));

                if (!RHS)
                {
                    return nullptr;
                }
            }
            LHS =
                std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
        }
    }

    // expression
    //  ::= primary binoprhs
    static std::unique_ptr<ExprAST> ParseExpression()
    {
        auto lhs = ParsePrimary();

        if (!lhs)
        {
            return nullptr;
        }
        return ParseBinOpRHS(0, std::move(lhs));
    }

    // prototype
    //  ::= id '(' id* ')'
    static std::unique_ptr<PrototypeAST> ParsePrototype()
    {
        if (CurTok != tok_identifier)
        {
            return LogErrorP("expected function name in prototype");
        }
        std::string FnName = IdentifierStr;
        getNextToken();

        if (CurTok != '(')
        {
            return LogErrorP("expected '(' in prototype");
        }

        // read list of argument names
        std::vector<std::string> ArgNames;

        while (getNextToken() == tok_identifier)
        {
            ArgNames.push_back(IdentifierStr);
        }
        if (CurTok != ')')
        {
            return LogErrorP("Expected ')' in prototype");
        }
        // success
        getNextToken(); // eat ')'
        return std::make_unique<PrototypeAST>(std::move(FnName), std::move(ArgNames));
    }

    // definition ::= 'def' prototype expression
    static std::unique_ptr<FunctionAST> ParseDefinition()
    {
        getNextToken(); // eat def
        auto Proto = ParsePrototype();

        if (!Proto)
        {
            return nullptr;
        }
        if (auto E = ParseExpression())
        {
            return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
        }
        return nullptr;
    }

    // external ::= 'extern' prototype
    static std::unique_ptr<PrototypeAST> ParseExtern()
    {
        getNextToken(); // eat extern
        return ParsePrototype();
    }

    // toplevelexpr := expression
    static std::unique_ptr<FunctionAST> ParseTopLevelExpr()
    {
        if (auto E = ParseExpression())
        {
            // make anonymous proto
            auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>{});
            return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
        }
        return nullptr;
    }

    static void HandleDefinition()
    {
        if (ParseDefinition())
        {
            fprintf(stdout, "parsed a function definition.\n");
        }
        else
        {
            // skip token for error recovery.
            getNextToken();
        }
    }

    static void HandleExtern()
    {
        if (ParseExtern())
        {
            fprintf(stdout, "parsed an extern\n");
        }
        else
        {
            // skip token for error recovery.
            getNextToken();
        }
    }

    static void HandleTopLevelExpression()
    {
        // Evaluate a top-level expression into an anonymous function.
        if (ParseTopLevelExpr())
        {
            fprintf(stdout, "parsed a top-level expr\n");
        }
        else
        {
            // skip token for error recovery.
            getNextToken();
        }
    }

    // top := definition | external | expression
    static void MainLoop()
    {
        while (true)
        {
            fprintf(stdout, "ready> ");
            switch (CurTok)
            {
            case tok_eof:
                return;
            case ';': // ignore top level semicolons
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
            }
        }
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_ORIGINAL_PARSER_HPP