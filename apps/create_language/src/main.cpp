#include <print>
#include <string_view>

#include "kaleidoscope/buffer_reader.hpp"
#include "kaleidoscope/lexer.hpp"
#include "kaleidoscope/token_formatter.hpp"

int main(int, char **)
{
  // kaleidoscope::gettok();

  kaleidoscope::lexer lexer;
  kaleidoscope::buffer_reader reader{"123"};
  const auto token = lexer.get_token(reader);

  if (token.has_value())
  {
    if (std::holds_alternative<kaleidoscope::number_token>(token.value()))
    {
      std::println("token: {}",
                   std::get<kaleidoscope::number_token>(token.value()));
    }
  }
}