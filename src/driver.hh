#include <cstdio>
#include <cstring>
#include <string>

#include "ast.hh"
#include "parser.hh"
#include "error.hh"
#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
YY_DECL;
extern FILE *yyin;

struct driver {
    ast::program program_ast;
    yy::location location;
    std::unordered_map<std::string, ast::identifier> symbols_map;
    std::vector<std::string> symbols_list;
    std::string filename;
    void parse() {
        location.initialize(&filename);
        scan_begin();
        yy::parser parser(*this);
        parser.parse();
        scan_end();
    }

    void scan_begin() {
        yyin = fopen(filename.c_str(), "r");
        if (!yyin) {
            error("cannot open", filename, ":", std::strerror(errno));
        }
    }

    void scan_end() {
        fclose(yyin);
    }
};
