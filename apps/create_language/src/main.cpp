#include <print>

#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/token_formatter.hpp"

int main(int, char **)
{
  kaleidoscope::gettok();

  kaleidoscope::lexer lexer;
  const auto token = lexer.get_token();

  if (token.has_value())
  {
    if (std::holds_alternative<kaleidoscope::number_token>(token.value()))
    {
      std::println("token: {}", std::get<kaleidoscope::number_token>(token.value()));
    }
  }
}