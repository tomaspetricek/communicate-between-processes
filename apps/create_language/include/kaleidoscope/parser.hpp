#ifndef KALEIDOSCOPE_PARSER_HPP
#define KALEIDOSCOPE_PARSER_HPP

#include <cassert>
#include <ctype.h>
#include <memory>
#include <print>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "etl/flat_map.h"

#include "kaleidoscope/abstract_syntax_tree.hpp"
#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/lexer.hpp"

// https://llvm.org/docs/tutorial/MyFirstLanguageFrontend/LangImpl02.html
namespace kaleidoscope
{
    bool is_empty(const ast::expression &expr)
    {
        return std::holds_alternative<std::monostate>(expr);
    }

    // helpers for error handling
    ast::expression log_error(const char *message)
    {
        fprintf(stderr, "error: %s\n", message);
        return ast::expression{};
    }

    // P - stands for prototype
    std::unique_ptr<ast::prototype> log_error_prototype(const char *message)
    {
        log_error(message);
        return nullptr;
    }

    static ast::expression parse_expression(kaleidoscope::lexer &lexer);

    // numberexpr ::= number
    static ast::expression parse_number_expression(kaleidoscope::lexer &lexer)
    {
        assert(std::holds_alternative<number_token>(lexer.current_token()));
        auto result = ast::make_expression<ast::number_expression>(
            std::get<number_token>(lexer.current_token()).number);
        lexer.get_next_token(); // consume the number
        return std::move(result);
    }

    // parentexpr ::= '(' expression ')'
    static ast::expression
    parse_parenthesis_expression(kaleidoscope::lexer &lexer)
    {
        lexer.get_next_token();              // eat (
        auto expr = parse_expression(lexer); // allows to handle recursive grammars

        if (is_empty(expr))
        {
            return ast::expression{};
        }
        if (!std::holds_alternative<right_parenthesis_token>(lexer.current_token()))
        {
            return log_error("expected ')'");
        }
        lexer.get_next_token(); // eat )
        return expr;            // once the parser constructs AST, parentheses are not needed
    }

    // identifierexpr
    //  ::= identifier
    //  ::= identifier '(' expression* ')'
    static ast::expression parse_identifier_expression(kaleidoscope::lexer &lexer)
    {
        assert(std::holds_alternative<identifier_token>(lexer.current_token()));
        std::string identifier =
            std::get<identifier_token>(lexer.current_token()).identifier;
        lexer.get_next_token(); // eat identifier

        if (!std::holds_alternative<left_parenthesis_token>(
                lexer.current_token()))
        { // simple variable reference // look ahead
            return ast::make_expression<ast::variable_expression>(
                std::move(identifier));
        }
        lexer.get_next_token(); // eat (
        std::vector<ast::expression> args;

        if (!std::holds_alternative<right_parenthesis_token>(lexer.current_token()))
        {
            while (true)
            {
                auto arg = parse_expression(lexer);

                if (!is_empty(arg))
                {
                    args.push_back(std::move(arg));
                }
                else
                {
                    return ast::expression{};
                }
                if (std::holds_alternative<right_parenthesis_token>(
                        lexer.current_token()))
                {
                    break;
                }
                if (!std::holds_alternative<comma_token>(lexer.current_token()))
                {
                    return log_error("Expected ')' or ',' in argument list");
                }
                lexer.get_next_token();
            }
        }
        lexer.get_next_token();
        return ast::make_expression<ast::call_expression>(std::move(identifier),
                                                          std::move(args));
    }

    // primary
    //  ::= identifierexpr
    //  ::= numberexpr
    //  ::= parenexpr
    static ast::expression parse_primary(kaleidoscope::lexer &lexer)
    {
        if (std::holds_alternative<identifier_token>(lexer.current_token()))
        {
            return parse_identifier_expression(lexer);
        }
        if (std::holds_alternative<number_token>(lexer.current_token()))
        {
            return parse_number_expression(lexer);
        }
        if (std::holds_alternative<left_parenthesis_token>(lexer.current_token()))
        {
            return parse_parenthesis_expression(lexer);
        }
        if (std::holds_alternative<eof_token>(lexer.current_token()))
        {
            return ast::expression{};
        }
        return log_error("unknown token when expecting an expression");
    }

    // holds precedence for each binary operator that is defined
    static std::unordered_map<char, int> binary_operator_precedence{
        {'<', 10}, {'+', 20}, {'-', 30}, {'*', 40} // highest
    };

    // get the precedence of the pending binary operator token
    static int get_token_precendence(kaleidoscope::lexer &lexer)
    {
        if (!std::holds_alternative<unknown_token>(lexer.current_token()) ||
            !isascii(std::get<unknown_token>(lexer.current_token()).value))
        {
            return -1;
        }
        // make sure that it's declared in the map
        const int token_precedence =
            binary_operator_precedence[std::get<unknown_token>(lexer.current_token())
                                           .value];

        if (token_precedence <= 0)
        {
            return -1;
        }
        return token_precedence;
    }

    ast::binary_operator convert_to_binary_operator(char op) noexcept
    {
        using operator_pair_t = std::pair<char, ast::binary_operator>;
        static const auto conversion = etl::make_flat_map<char, ast::binary_operator>(
            operator_pair_t{'<', ast::binary_operator::less_than},
            operator_pair_t{'+', ast::binary_operator::addition},
            operator_pair_t{'-', ast::binary_operator::subtraction},
            operator_pair_t{'*', ast::binary_operator::multiplication});

        const auto &it = conversion.find(op);
        assert(it != conversion.cend());
        return it->second;
    }

    // binoprhs
    //  ::= ('+' primary)
    static ast::expression parse_binaray_operation_rhs(kaleidoscope::lexer &lexer,
                                                       int expr_precendence,
                                                       ast::expression lhs)
    {
        // if it is binop find it's precedence
        while (true)
        {
            int token_precedence = get_token_precendence(lexer);

            // if this is a binop that binds at least as tightly as the current binop,
            // consume it, otherwise we are done
            if (token_precedence < expr_precendence)
            {
                return lhs;
            }
            // know that it is binop
            const auto op = std::get<unknown_token>(lexer.current_token()).value;
            lexer.get_next_token(); // eat binop

            // parse the primary expression after the binary operator
            auto rhs = parse_primary(lexer);

            if (is_empty(rhs))
            {
                return ast::expression{};
            }
            // if binop binds less tightly with rhs than the operator rhs, let
            // the pending operator take rhs as its lhs
            int next_precedence = get_token_precendence(lexer);

            if (token_precedence < next_precedence)
            {
                rhs = parse_binaray_operation_rhs(lexer, token_precedence + 1,
                                                  std::move(rhs));

                if (is_empty(rhs))
                {
                    return ast::expression{};
                }
            }
            lhs = ast::make_expression<ast::binary_expression>(
                convert_to_binary_operator(op), std::move(lhs), std::move(rhs));
        }
    }

    // expression
    //  ::= primary binoprhs
    static ast::expression parse_expression(kaleidoscope::lexer &lexer)
    {
        auto lhs = parse_primary(lexer);

        if (is_empty(lhs))
        {
            return lhs;
        }
        return parse_binaray_operation_rhs(lexer, 0, std::move(lhs));
    }

    // prototype
    //  ::= id '(' id* ')'
    static std::unique_ptr<ast::prototype>
    parse_prototype(kaleidoscope::lexer &lexer)
    {
        if (!std::holds_alternative<identifier_token>(lexer.current_token()))
        {
            return log_error_prototype("expected function name in prototype");
        }
        std::string function_name =
            std::get<identifier_token>(lexer.current_token()).identifier;
        lexer.get_next_token();

        if (!std::holds_alternative<left_parenthesis_token>(lexer.current_token()))
        {
            return log_error_prototype("expected '(' in prototype");
        }
        // read list of argument names
        std::vector<std::string> arg_names;

        while (std::holds_alternative<identifier_token>(lexer.get_next_token()))
        {
            arg_names.push_back(
                std::get<identifier_token>(lexer.current_token()).identifier);
        }
        if (!std::holds_alternative<right_parenthesis_token>(lexer.current_token()))
        {
            return log_error_prototype("expected ')' in prototype");
        }
        // success
        lexer.get_next_token(); // eat ')'
        return std::make_unique<ast::prototype>(std::move(function_name),
                                                std::move(arg_names));
    }

    // definition ::= 'def' prototype expression
    static std::unique_ptr<ast::function>
    parse_definition(kaleidoscope::lexer &lexer)
    {
        lexer.get_next_token(); // eat def
        auto proto = parse_prototype(lexer);

        if (!proto)
        {
            return nullptr;
        }
        auto expr = parse_expression(lexer);

        if (!is_empty(expr))
        {
            return std::make_unique<ast::function>(std::move(proto), std::move(expr));
        }
        return nullptr;
    }

    // external ::= 'extern' prototype
    static std::unique_ptr<ast::prototype>
    parse_extern(kaleidoscope::lexer &lexer)
    {
        lexer.get_next_token(); // eat extern
        return parse_prototype(lexer);
    }

    // toplevelexpr := expression
    static std::unique_ptr<ast::function>
    parse_top_level_expression(kaleidoscope::lexer &lexer)
    {
        auto expr = parse_expression(lexer);

        if (!is_empty(expr))
        {
            // make anonymous proto
            auto proto =
                std::make_unique<ast::prototype>("", std::vector<std::string>{});
            return std::make_unique<ast::function>(std::move(proto), std::move(expr));
        }
        return nullptr;
    }

    static void handle_definition(kaleidoscope::lexer &lexer)
    {
        if (parse_definition(lexer))
        {
            fprintf(stdout, "parsed a function definition\n");
        }
        else
        {
            fprintf(stderr, "failed parsing a function definition\n");
            lexer.get_next_token();
        }
    }

    static void handle_extern(kaleidoscope::lexer &lexer)
    {
        const auto extern_parsed = parse_extern(lexer);

        if (extern_parsed)
        {
            fprintf(stdout, "parsed an extern\n");
            std::println("name: {}", extern_parsed->name);

            for (const auto &arg : extern_parsed->args)
            {
                std::println("    arg: {}", arg);
            }
        }
        else
        {
            fprintf(stderr, "failed parsing an extern\n");
            lexer.get_next_token();
        }
    }

    static void handle_top_level_expression(kaleidoscope::lexer &lexer)
    {
        // Evaluate a top-level expression into an anonymous function.
        const auto expr = parse_top_level_expression(lexer);

        if (expr)
        {
            fprintf(stdout, "parsed a top-level expr\n");
        }
        else
        {
            fprintf(stderr, "failed parsing a top-level expr\n");
            lexer.get_next_token();
        }
    }

    // top := definition | external | expression
    static void main_loop(buffer_reader &reader)
    {
        kaleidoscope::lexer lexer{reader};
        lexer.get_next_token();

        while (true)
        {
            if (std::holds_alternative<eof_token>(lexer.current_token()))
            {
                return;
            }
            else if (std::holds_alternative<unknown_token>(lexer.current_token()) &&
                     std::get<unknown_token>(lexer.current_token()).value == ';')
            {
                lexer.get_next_token();
            }
            else if (std::holds_alternative<def_token>(lexer.current_token()))
            {
                handle_definition(lexer);
            }
            else if (std::holds_alternative<extern_token>(lexer.current_token()))
            {
                handle_extern(lexer);
            }
            else
            {
                handle_top_level_expression(lexer);
            }
        }
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_PARSER_HPP