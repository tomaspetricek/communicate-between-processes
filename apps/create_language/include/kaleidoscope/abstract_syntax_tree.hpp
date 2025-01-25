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
        class number_expression : public expression
        {
            double value_{0};

        public:
            explicit number_expression(double value) : value_{value} {}
        };

        class variable_expression : public expression
        {
            std::string name_;

        public:
            explicit variable_expression(const std::string &name) : name_{name} {}
        };

        // binary operator
        class binary_expression : public expression
        {
            char op_;
            std::unique_ptr<expression> lhs_, rhs_;

        public:
            explicit binary_expression(char op, std::unique_ptr<expression> lhs,
                                       std::unique_ptr<expression> rhs)
                : op_{op}, lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}
        };

        // function calls
        class call_expression : public expression
        {
            std::string calle_;
            std::vector<std::unique_ptr<expression>> args_;

        public:
            explicit call_expression(const std::string &calle,
                                     std::vector<std::unique_ptr<expression>> args)
                : calle_(calle), args_(std::move(args)) {}
        };

        // repersents prototype for a function
        class prototype
        {
            std::string name_;
            std::vector<std::string> args_;

        public:
            explicit prototype(const std::string &name, std::vector<std::string> args)
                : name_{name}, args_{std::move(args)} {}

            const std::string &get_name() const { return name_; }
        };

        // represents function definition itself
        class function
        {
            std::unique_ptr<prototype> proto_;
            std::unique_ptr<expression> body_;

        public:
            explicit function(std::unique_ptr<prototype> proto, std::unique_ptr<expression> body)
                : proto_{std::move(proto)}, body_{std::move(body)} {}
        };
}

#endif // KALEIDOSCOPE_ABSTRACT_SYNTAX_TREE_HPP