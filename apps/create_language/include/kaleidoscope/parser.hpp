#ifndef KALEIDOSCOPE_PARSER_HPP
#define KALEIDOSCOPE_PARSER_HPP

#include <ctype.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "kaleidoscope/original/lexer.hpp"

// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
namespace kaleidoscope
{
    namespace ast
    {
        struct expression
        {
            virtual ~expression() = default;
        };

        // numeric literal
        class number_expression : public expression
        {
            double value_{0};

        public:
            explicit number_expression(double value) : value_{value} {}
        };

        class variable_expression : public expression
        {
            std::string name_;

        public:
            explicit variable_expression(const std::string &name) : name_{name} {}
        };

        // binary operator
        class binary_expression : public expression
        {
            char op_;
            std::unique_ptr<expression> lhs_, rhs_;

        public:
            explicit binary_expression(char op, std::unique_ptr<expression> lhs,
                                       std::unique_ptr<expression> rhs)
                : op_{op}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
        };

        // function calls
        class call_expression : public expression
        {
            std::string calle_;
            std::vector<std::unique_ptr<expression>> args_;

        public:
            explicit call_expression(const std::string &calle,
                                     std::vector<std::unique_ptr<expression>> args)
                : calle_(calle), args_(std::move(args)) {}
        };

        // repersents prototype for a function
        class prototype
        {
            std::string name_;
            std::vector<std::string> args_;

        public:
            explicit prototype(const std::string &name, std::vector<std::string> args)
                : name_{name}, args_{std::move(args)} {}

            const std::string &get_name() const { return name_; }
        };

        // represents function definition itself
        class function
        {
            std::unique_ptr<prototype> proto_;
            std::unique_ptr<expression> body_;

        public:
            explicit function(std::unique_ptr<prototype> proto, std::unique_ptr<expression> body)
                : proto_{std::move(proto)}, body_{std::move(body)} {}
        };
    } // namespace ast

    // provide simple token buffer
    static int current_token; // is current token that the parser is looking at

    // updates current token
    static int get_next_token() { return current_token = gettok(); }

    // helpers for error handling
    std::unique_ptr<ast::expression> log_error(const char *message)
    {
        fprintf(stderr, "error: %s\n", message);
        return nullptr;
    }

    // P - stands for prototype
    std::unique_ptr<ast::prototype> log_error_prototype(const char *message)
    {
        log_error(message);
        return nullptr;
    }

    static std::unique_ptr<ast::expression> parse_expression();

    // numberexpr ::= number
    static std::unique_ptr<ast::expression> parse_number_expression()
    {
        auto result = std::make_unique<ast::number_expression>(NumVal);
        get_next_token(); // consume the number
        return std::move(result);
    }

    // parentexpr ::= '(' expression ')'
    static std::unique_ptr<ast::expression> parse_parenthesis_expression()
    {
        get_next_token();             // eat (
        auto expr = parse_expression(); // allows to handle recursive grammars

        if (!expr)
        {
            return nullptr;
        }

        if (current_token != ')')
        {
            return log_error("expected ')'");
        }
        get_next_token(); // eat )
        return expr;       // once the parser constructs AST, parentheses are not needed
    }

    // identifierexpr
    //  ::= identifier
    //  ::= identifier '(' expression* ')'
    static std::unique_ptr<ast::expression> parse_identifier_expression()
    {
        std::string identifier = IdentifierStr;
        get_next_token(); // eat identifier

        if (current_token != '(')
        { // simple variable reference // look ahead
            return std::make_unique<ast::variable_expression>(identifier);
        }
        get_next_token(); // eat (
        std::vector<std::unique_ptr<ast::expression>> args;

        if (current_token != ')')
        {
            while (true)
            {
                if (auto arg = parse_expression())
                {
                    args.push_back(std::move(arg));
                }
                else
                {
                    return nullptr;
                }

                if (current_token == ')')
                {
                    break;
                }

                if (current_token != ',')
                {
                    return log_error("Expected ')' or ',' in argument list");
                }
                get_next_token();
            }
        }
        get_next_token();
        return std::make_unique<ast::call_expression>(std::move(identifier), std::move(args));
    }

    // primary
    //  ::= identifierexpr
    //  ::= numberexpr
    //  ::= parenexpr
    static std::unique_ptr<ast::expression> parse_primary()
    {
        switch (current_token)
        {
        default:
            return log_error("unknown token when expecting an expression");
        case tok_identifier:
            return parse_identifier_expression();
        case tok_number:
            return parse_number_expression();
        case '(':
            return parse_parenthesis_expression();
        }
    }

    // holds precedence for each binary operator that is defined
    static std::unordered_map<char, int> binary_operator_precedence{
        {'<', 10}, {'+', 20}, {'-', 30}, {'*', 40} // highest
    };

    // get the precedence of the pending binary operator token
    static int get_token_precendence()
    {
        if (!isascii(current_token))
        {
            return -1;
        }
        // make sure that it's declared in the map
        const int token_precedence = binary_operator_precedence[current_token];

        if (token_precedence <= 0)
        {
            return -1;
        }
        return token_precedence;
    }

    // binoprhs
    //  ::= ('+' primary)
    static std::unique_ptr<ast::expression> parse_binaray_operation_rhs(int expr_precendence, std::unique_ptr<ast::expression> lhs)
    {
        // if it is binop find it's precedence
        while (true)
        {
            int token_precedence = get_token_precendence();

            // if this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done
            if (token_precedence < expr_precendence)
            {
                return lhs;
            }
            // know that it is binop
            int binary_operator = current_token;
            get_next_token(); // eat binop

            // parse the primary expression after the binary operator
            auto rhs = parse_primary();

            if (!rhs)
            {
                return nullptr;
            }
            // if binop binds less tightly with rhs than the operator rhs, let
            // the pending operator take rhs as its lhs
            int next_precedence = get_token_precendence();

            if (token_precedence < next_precedence)
            {
                rhs = parse_binaray_operation_rhs(token_precedence + 1, std::move(rhs));

                if (!rhs)
                {
                    return nullptr;
                }
            }
            lhs = std::make_unique<ast::binary_expression>(binary_operator, std::move(lhs),
                                                           std::move(rhs));
        }
    }

    // expression
    //  ::= primary binoprhs
    static std::unique_ptr<ast::expression> parse_expression()
    {
        auto lhs = parse_primary();

        if (!lhs)
        {
            return nullptr;
        }
        return parse_binaray_operation_rhs(0, std::move(lhs));
    }

    // prototype
    //  ::= id '(' id* ')'
    static std::unique_ptr<ast::prototype> parse_prototype()
    {
        if (current_token != tok_identifier)
        {
            return log_error_prototype("expected function name in prototype");
        }
        std::string function_name = IdentifierStr;
        get_next_token();

        if (current_token != '(')
        {
            return log_error_prototype("expected '(' in prototype");
        }

        // read list of argument names
        std::vector<std::string> arg_names;

        while (get_next_token() == tok_identifier)
        {
            arg_names.push_back(IdentifierStr);
        }
        if (current_token != ')')
        {
            return log_error_prototype("Expected ')' in prototype");
        }
        // success
        get_next_token(); // eat ')'
        return std::make_unique<ast::prototype>(std::move(function_name),
                                                std::move(arg_names));
    }

    // definition ::= 'def' prototype expression
    static std::unique_ptr<ast::function> parse_definition()
    {
        get_next_token(); // eat def
        auto proto = parse_prototype();

        if (!proto)
        {
            return nullptr;
        }
        if (auto expr = parse_expression())
        {
            return std::make_unique<ast::function>(std::move(proto), std::move(expr));
        }
        return nullptr;
    }

    // external ::= 'extern' prototype
    static std::unique_ptr<ast::prototype> parse_extern()
    {
        get_next_token(); // eat extern
        return parse_prototype();
    }

    // toplevelexpr := expression
    static std::unique_ptr<ast::function> parse_top_level_expression()
    {
        if (auto expr = parse_expression())
        {
            // make anonymous proto
            auto proto =
                std::make_unique<ast::prototype>("", std::vector<std::string>{});
            return std::make_unique<ast::function>(std::move(proto), std::move(expr));
        }
        return nullptr;
    }

    static void handle_definition()
    {
        if (parse_definition())
        {
            fprintf(stdout, "parsed a function definition.\n");
        }
        else
        {
            // skip token for error recovery.
            get_next_token();
        }
    }

    static void handle_extern()
    {
        if (parse_extern())
        {
            fprintf(stdout, "parsed an extern\n");
        }
        else
        {
            // skip token for error recovery.
            get_next_token();
        }
    }

    static void handle_top_level_expression()
    {
        // Evaluate a top-level expression into an anonymous function.
        if (parse_top_level_expression())
        {
            fprintf(stdout, "parsed a top-level expr\n");
        }
        else
        {
            // skip token for error recovery.
            get_next_token();
        }
    }

    // top := definition | external | expression
    static void main_loop()
    {
        while (true)
        {
            fprintf(stdout, "ready> ");
            switch (current_token)
            {
            case tok_eof:
                return;
            case ';': // ignore top level semicolons
                get_next_token();
                break;
            case tok_def:
                handle_definition();
                break;
            case tok_extern:
                handle_extern();
                break;
            default:
                handle_top_level_expression();
                break;
            }
        }
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_PARSER_HPP