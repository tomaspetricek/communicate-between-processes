#ifndef KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_FORMATTER_HPP
#define KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_FORMATTER_HPP

#include <print>

#include "kaleidoscope/abstract_syntax_tree.hpp"

namespace kaleidoscope
{
    void print(const ast::node& node) noexcept {
        std::println("node: ");
    }
} // namespace kaleidoscope

#endif // KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_FORMATTER_HPP