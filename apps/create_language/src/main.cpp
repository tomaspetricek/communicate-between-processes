#include <print>
#include <string_view>

#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/token_formatter.hpp"

int main(int, char **)
{
  // kaleidoscope::gettok();

  kaleidoscope::lexer lexer;
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

  kaleidoscope::token_t token;
  std::println("tokens:");

  do
  {
    token = lexer.get_token(reader);
    std::println("{}", token);
  } while (!std::holds_alternative<kaleidoscope::eof_token>(token));
}