#ifndef KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP
#define KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP

#include <memory>
#include <string>
#include <vector>

namespace kaleidoscope::ast {
        struct expression
        {
            virtual ~expression() = default;
        };

        // numeric literal
        struct number_expression : public expression
        {
            double value{0};

            explicit number_expression(double value_) noexcept : value{value_} {}
        };

        struct variable_expression : public expression
        {
            std::string name;

            explicit variable_expression(const std::string &name_) : name{name_} {}
        };

        // binary operator
        struct binary_expression : public expression
        {
            char op;
            std::unique_ptr<expression> lhs, rhs;

            explicit binary_expression(char op_, std::unique_ptr<expression> lhs_,
                                       std::unique_ptr<expression> rhs_)
                : op{op_}, lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
        };

        // function calls
        struct call_expression : public expression
        {
            std::string calle;
            std::vector<std::unique_ptr<expression>> args;

            explicit call_expression(const std::string &calle_,
                                     std::vector<std::unique_ptr<expression>> args_)
                : calle(calle_), args(std::move(args_)) {}
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
            std::unique_ptr<expression> body;

            explicit function(std::unique_ptr<prototype> proto_, std::unique_ptr<expression> body_)
                : proto{std::move(proto_)}, body{std::move(body_)} {}
        };
}

#endif // KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP