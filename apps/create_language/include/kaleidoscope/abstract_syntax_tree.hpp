#ifndef KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP
#define KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace kaleidoscope
{
    namespace ast
    {
        struct expression;

        struct expression_deleter
        {
            void operator()(expression *expr);
        };
        using expression_ptr = std::unique_ptr<expression, expression_deleter>;

        // numeric literal
        struct number_expression
        {
            double value{0};

            explicit number_expression(double value_) noexcept : value{value_} {}
        };

        struct variable_expression
        {
            std::string name;

            explicit variable_expression(const std::string &name_) : name{name_} {}
        };

        // binary operator
        struct binary_expression
        {
            char op;
            expression_ptr lhs, rhs;

            explicit binary_expression(char op_, expression_ptr lhs_, expression_ptr rhs_)
                : op{op_}, lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
        };

        // function calls
        struct call_expression
        {
            std::string calle;
            std::vector<expression_ptr> args;

            explicit call_expression(const std::string &calle_,
                                     std::vector<expression_ptr> args_)
                : calle(calle_), args(std::move(args_)) {}
        };

        struct expression : std::variant<number_expression, variable_expression,
                                         binary_expression, call_expression>
        {
            using std::variant<number_expression, variable_expression, binary_expression,
                               call_expression>::variant;
        };

        // repersents prototype for a function
        struct prototype
        {
            std::string name;
            std::vector<std::string> args;

            explicit prototype(const std::string &name_, std::vector<std::string> args_)
                : name{name_}, args{std::move(args_)} {}
        };

        // represents function definition itself
        struct function
        {
            std::unique_ptr<prototype> proto;
            expression_ptr body;

            explicit function(std::unique_ptr<prototype> proto_, expression_ptr body_)
                : proto{std::move(proto_)}, body{std::move(body_)} {}
        };
    } // namespace ast

    template <class Expression, class... Args>
    ast::expression_ptr make_expression(Args &&...args)
    {
        return ast::expression_ptr{
            new ast::expression{Expression{std::forward<Args>(args)...}}};
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP