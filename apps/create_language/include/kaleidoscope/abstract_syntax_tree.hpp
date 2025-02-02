#ifndef KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP
#define KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "core/variant_cast.hpp"

namespace kaleidoscope
{
  namespace ast
  {
    struct number_expression;
    struct variable_expression;
    struct binary_expression;
    struct call_expression;

    using expression =
        std::variant<std::monostate, std::unique_ptr<number_expression>,
                     std::unique_ptr<variable_expression>,
                     std::unique_ptr<binary_expression>,
                     std::unique_ptr<call_expression>>;

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
      expression lhs, rhs;

      explicit binary_expression(char op_, expression lhs_, expression rhs_)
          : op{op_}, lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
    };

    // function calls
    struct call_expression
    {
      std::string calle;
      std::vector<expression> args;

      explicit call_expression(const std::string &calle_,
                               std::vector<expression> args_)
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
      expression body;

      explicit function(std::unique_ptr<prototype> proto_, expression body_)
          : proto{std::move(proto_)}, body{std::move(body_)} {}
    };

    using node =
        std::variant<std::monostate, std::unique_ptr<number_expression>,
                     std::unique_ptr<variable_expression>,
                     std::unique_ptr<binary_expression>,
                     std::unique_ptr<call_expression>, std::unique_ptr<prototype>,
                     std::unique_ptr<function>>;

    template <class Expression, class... Args>
    ast::expression make_expression(Args &&...args)
    {
      return ast::expression{
          std::make_unique<Expression>(std::forward<Args>(args)...)};
    }

    ast::node make_node(ast::expression expression)
    {
      return core::variant_cast(std::move(expression));
    }
  } // namespace ast
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP