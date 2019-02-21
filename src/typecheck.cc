#include <vector>
#include <unordered_map>
#include <memory>
#include <variant>
#include <functional>
#include <stack>

#include "ast.hh"

#include <string>
#include <iostream>
static void error(std::string s) {
    std::cerr << s << std::endl;
    exit(EXIT_FAILURE);
}

struct typecheck_context {
    ast::type current_function_returntype;
    ast::type current_loop_returntype;
    std::unordered_map<ast::identifier, ast::function_def> functions;
    using scope = std::unordered_map<ast::identifier, ast::type>;
    std::stack<scope> scopes;
    std::vector<std::string> symbols;
};

struct typecheck_fn {
    typecheck_context& context;
    ast::type operator()(ast::program& program) {
        context.scopes.push({});
        for (auto& statement: program.statements) {
            std::invoke(*this, statement);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::statement& statement) {
        return std::visit(*this, statement.statement);
    }
    ast::type operator()(ast::block& block) {
        for (auto& statement: block.statements) {
            std::invoke(*this, statement);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::if_statement& if_statement) {
        for (auto& condition: if_statement.conditions) {
            if (std::invoke(*this, condition) != ast::type::t_bool) {
                error("if statement condition not a boolean");
            }
        }
        for (auto& block: if_statement.blocks) {
            std::invoke(*this, block);
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::for_loop& for_loop) {
        if (std::invoke(*this, for_loop.condition) != ast::type::t_bool) {
            error("for loop condition not a boolean");
        }
        std::invoke(*this, for_loop.block);
        return ast::type::t_void;
    }
    ast::type operator()(ast::while_loop& while_loop) {
        if (std::invoke(*this, while_loop.condition) != ast::type::t_bool) {
            error("while loop condition not a boolean");
        }
        std::invoke(*this, while_loop.block);
        return ast::type::t_void;
    }
    ast::type operator()(ast::function_def& function_def) {
        auto& x = context.scopes.top()[function_def.identifier];
        if (x) {
            error("function already defined");
        }
        x = function_def.returntype;
        context.current_function_returntype = x;
        std::invoke(*this, function_def.block);
        //TODO put the signature somewhere to typecheck function calls
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_return& s_return) {
        ast::type x = std::invoke(*this, s_return.expression);
        if (x != context.current_function_returntype) {
            error("return type does not match defined function return type");
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_break& s_break) {
        //TODO check the return type matches the loop return type
        //std::invoke(*this, s_break.expression)
        return ast::type::t_void;
    }
    ast::type operator()(ast::s_continue& s_continue) {
        return ast::type::t_void;
    }
    ast::type operator()(ast::variable_def& variable_def) {
        auto& x = context.scopes.top()[variable_def.identifier];
        if (x) {
            error("variable already defined");
        }
        x = std::invoke(*this, variable_def.expression);
        if (x != variable_def.type) {
            error("type mismatch in variable definition");
        }
        return ast::type::t_void;
    }
    ast::type operator()(ast::assignment& assignment) {
        //TODO use the variable type information to check the expression type
        ast::type variable = context.scopes.top()[assignment.identifier];
        if (!variable) {
            error("variable used before being defined");
        }
        ast::type value = std::invoke(*this, assignment.expression);
        if (value != variable) {
            error("type mismatch in assignment");
        }
        return ast::type::t_void;
    }

    ast::type operator()(ast::expression& expression) {
        return std::visit(*this, expression.expression);
    }
    ast::type operator()(ast::identifier& identifier) {
        ast::type variable = context.scopes.top()[identifier];
        if (!variable) {
            error("variable used before being defined");
        }
        return variable;
    }
    ast::type operator()(ast::literal& literal) {
        struct literal_visitor {
            typecheck_context& context;
            ast::type operator()(double& x) {
                return ast::type::f64;
            }
            ast::type operator()(uint64_t& x) {
                return ast::type::u64;
            }
            ast::type operator()(bool& x) {
                return ast::type::t_bool;
            }
        };
        return std::visit(literal_visitor{context}, literal.literal);
    }
    ast::type operator()(std::unique_ptr<ast::function_call>& function_call) {
        //TODO check the function signature
        auto& x = context.scopes.top()[function_call->identifier];
        return x;
    }
    ast::type operator()(std::unique_ptr<ast::binary_operator>& binary_operator) {
        ast::type l = std::invoke(*this, binary_operator->l);
        ast::type r = std::invoke(*this, binary_operator->r);
        switch (binary_operator->binary_operator) {
            case ast::binary_operator::A_ADD:
            case ast::binary_operator::A_SUB:
            case ast::binary_operator::A_MUL:
            case ast::binary_operator::A_DIV:
            case ast::binary_operator::A_MOD:
                if (l != r) {
                    error("LHS and RHS of arithmetic operator are not of the same type");
                }
                if (!ast::type_is_number(l)) {
                    error("LHS and RHS of arithmetic operator are not numbers");
                }
                return l;
            case ast::binary_operator::B_SHL:
            case ast::binary_operator::B_SHR:
                if (!ast::type_is_integer(l)) {
                    error("LHS of shift operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error("RHS of shift operator is not an integer");
                }
                return l;
            case ast::binary_operator::B_AND:
            case ast::binary_operator::B_XOR:
            case ast::binary_operator::B_OR:
                if (l != r) {
                    error("LHS and RHS of bitwise operator are not of the same type");
                }
                if (!ast::type_is_integer(l)) {
                    error("LHS of bitwise operator is not an integer");
                }
                if (!ast::type_is_integer(r)) {
                    error("RHS of bitwise operator is not an integer");
                }
                return l;
            case ast::binary_operator::L_AND:
            case ast::binary_operator::L_OR:
                if (!ast::type_is_bool(l)) {
                    error("LHS of logical operator is not a boolean");
                }
                if (!ast::type_is_bool(r)) {
                    error("RHS of logical operator is not a boolean");
                }
                return l;
            case ast::binary_operator::C_EQ:
            case ast::binary_operator::C_NE:
                if (l != r) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                return ast::type::t_bool;
            case ast::binary_operator::C_GT:
            case ast::binary_operator::C_GE:
            case ast::binary_operator::C_LT:
            case ast::binary_operator::C_LE:
                if (l != r) {
                    error("LHS and RHS of comparison operator are not of the same type");
                }
                if (!ast::type_is_number(l)) {
                    error("LHS and RHS of comparison operator are not numbers");
                }
                return ast::type::t_bool;
        }
        assert(false);
    }
    ast::type operator()(std::unique_ptr<ast::unary_operator>& unary_operator) {
        ast::type r = std::invoke(*this, unary_operator->r);
        switch (unary_operator->unary_operator) {
            case ast::unary_operator::B_NOT:
                if (!ast::type_is_integer(r)) {
                    error("RHS of bitwise negation is not an integer");
                }
                return r;
            case ast::unary_operator::L_NOT:
                if (!ast::type_is_bool(r)) {
                    error("RHS of bitwise negation is not boolean");
                }
                return r;
        }
        assert(false);
    }
};

void typecheck(typecheck_context &context, ast::program &program) {
    std::invoke(typecheck_fn{context}, program);
}
