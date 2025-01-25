#include "kaleidoscope/abstract_syntax_tree.hpp"

namespace kaleidoscope::ast
{
    void expression_deleter::operator()(expression *expr) {
        delete expr;
    }
}