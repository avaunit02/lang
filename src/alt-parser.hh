#pragma once

#include "ast.hh"

ast::program parse(std::string filename);
void test_grammar();
