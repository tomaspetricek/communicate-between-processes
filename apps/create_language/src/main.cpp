#include <memory>
#include <print>
#include <string_view>
#include <utility>

#include "kaleidoscope/abstract_syntax_tree.hpp"
#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/parser.hpp"
#include "kaleidoscope/token_formatter.hpp"

int main(int, char **)
{
  std::string code{"# Compute the x'th fibonacci number.\n"
                   "def fib(x)\n"
                   "if x < 3 then\n"
                   "  1\n"
                   "else\n"
                   "  fib(x-1)+fib(x-2)\n"
                   "\n"
                   "# This expression will compute the 40th number.\n"
                   "fib(40)\n"};
  code += static_cast<char>(EOF);
  kaleidoscope::buffer_reader reader{code};
  kaleidoscope::lexer lexer{reader};

  kaleidoscope::token_t token;
  std::println("tokens:");

  do
  {
    token = lexer.get_token();
    std::println("{}", token);
  } while (!std::holds_alternative<kaleidoscope::eof_token>(token));

  auto lhs = kaleidoscope::make_expression<kaleidoscope::ast::variable_expression>("x");
  auto rhs = kaleidoscope::make_expression<kaleidoscope::ast::variable_expression>("y");
  auto result = std::make_unique<kaleidoscope::ast::binary_expression>('+', std::move(lhs), std::move(rhs));

  std::string sample2{
      "extern sin(arg);"
      "extern cos(arg);"
      "extern atan2(arg1 arg2);"
      ""
      "atan2(sin(.4), cos(42))"};
  kaleidoscope::buffer_reader sample2_reader{sample2};
  kaleidoscope::main_loop(sample2_reader);
}