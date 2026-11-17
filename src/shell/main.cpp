#include "codegen.h"

#include <cstddef>
#include <cstdio>
#include <cstring>

extern "C" {
#include "diagnostic.h"
}

extern "C" {
#include "smallcpp.tab.h"

extern FILE *yyin;
extern int yylineno;
extern int yycolumn;

static int error_count;

void yyerror(const char *message)
{
    int line = yylloc.first_line > 0 ? yylloc.first_line : 1;
    int column = yylloc.first_column > 0 ? yylloc.first_column : 1;

    smallcpp_diag_report(line, column, message);
    error_count++;
}
}

int main(int argc, char **argv)
{
    int status;

    if (argc != 3) {
        std::fprintf(stderr, "usage: %s input.scpp output.bc|program\n", argv[0]);
        return 2;
    }
    smallcpp_ast = NULL;
    error_count = 0;
    yylineno = 1;
    yycolumn = 1;
    yylloc.first_line = 1;
    yylloc.first_column = 1;
    yylloc.last_line = 1;
    yylloc.last_column = 1;
    smallcpp_diag_load(argv[1]);
    yyin = std::fopen(argv[1], "r");
    if (yyin == NULL) {
        std::perror(argv[1]);
        return 2;
    }
    status = yyparse();
    std::fclose(yyin);
    if (status != 0 || error_count != 0 || smallcpp_ast == NULL) {
        ast_free(smallcpp_ast);
        smallcpp_ast = NULL;
        return 1;
    }
    std::size_t length = std::strlen(argv[2]);
    if (length >= 3 && std::strcmp(argv[2] + length - 3, ".bc") == 0)
        status = compile_to_bitcode(smallcpp_ast, argv[2]);
    else
        status = compile_to_executable(smallcpp_ast, argv[2]);
    ast_free(smallcpp_ast);
    smallcpp_ast = NULL;
    return status;
}
