#include "ast.hh"
#include <string>

extern ast::program *program_ast;

void yyerror(std::string s);
size_t lookup_or_insert(char* c);
int parse_integer(char *s, size_t base);
ast::expression* new_bin_op(ast::expression* l, ast::expression* r, ast::binary_operator::op);
