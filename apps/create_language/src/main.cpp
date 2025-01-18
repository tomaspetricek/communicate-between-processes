#include <print>

#include "kaleidoscope/lexer.hpp"

int main(int, char **)
{
  std::println("language");
  kaleidoscope::gettok();

  kaleidoscope::lexer lexer;
  const auto token = lexer.get_token();

  if (token.has_value())
  {
    if (std::holds_alternative<kaleidoscope::number_token>(token.value()))
    {
      const auto number =
          std::get<kaleidoscope::number_token>(token.value()).number;
      std::println("value: {}", number);
    }
  }
}