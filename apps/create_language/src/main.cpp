#include <print>
#include <string_view>

#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/token_formatter.hpp"

// ToDo: simplify
void print(const kaleidoscope::token_t &token)
{
  if (std::holds_alternative<kaleidoscope::eof_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::eof_token>(token));
  }
  else if (std::holds_alternative<kaleidoscope::def_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::def_token>(token));
  }
  else if (std::holds_alternative<kaleidoscope::extern_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::extern_token>(token));
  }
  else if (std::holds_alternative<kaleidoscope::identifier_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::identifier_token>(token));
  }
  else if (std::holds_alternative<kaleidoscope::number_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::number_token>(token));
  }
  else if (std::holds_alternative<kaleidoscope::unknown_token>(token))
  {
    std::println("token: {}", std::get<kaleidoscope::unknown_token>(token));
  }
}

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
  const auto token = lexer.get_token(reader);
  print(token);
}