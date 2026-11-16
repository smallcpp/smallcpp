#include "ast.h"
#include "smallcpp.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yyparse(void);
extern FILE *yyin;
extern int yylineno;
extern int yycolumn;

static const char *input_path;
static int error_count;

void yyerror(const char *message)
{
    int line = yylloc.first_line > 0 ? yylloc.first_line : 1;
    int column = yylloc.first_column > 0 ? yylloc.first_column : 1;

    fprintf(stderr, "%s:%d:%d: %s\n", input_path, line, column, message);
    error_count++;
}

static int ends_with(const char *path, const char *suffix)
{
    size_t path_len = strlen(path);
    size_t suffix_len = strlen(suffix);
    return path_len >= suffix_len
        && strcmp(path + path_len - suffix_len, suffix) == 0;
}

static int require_kind(const AstNode *root, AstKind kind)
{
    if (ast_contains(root, kind))
        return 1;
    fprintf(stderr, "syntax tree has no %s node\n", ast_kind_name(kind));
    return 0;
}

static int tree_is_complete(const char *path, const AstNode *root)
{
    int ok = 1;

    if (root == NULL) {
        fprintf(stderr, "no syntax tree\n");
        return 0;
    }
    ok = require_kind(root, AST_TRANSLATION_UNIT);
    if (ends_with(path, "print-format.scpp")) {
        ok = require_kind(root, AST_PRINT) && ok;
        ok = require_kind(root, AST_FORMAT) && ok;
        return ok;
    }
    if (!ends_with(path, "accept.scpp"))
        return ok;
    ok = require_kind(root, AST_NAMESPACE) && ok;
    ok = require_kind(root, AST_CLASS) && ok;
    ok = require_kind(root, AST_METHOD) && ok;
    ok = require_kind(root, AST_TRY) && ok;
    ok = require_kind(root, AST_CATCH) && ok;
    ok = require_kind(root, AST_THROW) && ok;
    ok = require_kind(root, AST_NEW) && ok;
    return ok;
}

int main(int argc, char **argv)
{
    int status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 2;
    }
    input_path = argv[1];
    smallcpp_ast = NULL;
    yylineno = 1;
    yycolumn = 1;
    yylloc.first_line = 1;
    yylloc.first_column = 1;
    yylloc.last_line = 1;
    yylloc.last_column = 1;
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror(argv[1]);
        return 2;
    }
    status = yyparse();
    fclose(yyin);
    if (status != 0)
        return 1;
    if (!tree_is_complete(input_path, smallcpp_ast)) {
        ast_free(smallcpp_ast);
        smallcpp_ast = NULL;
        return 1;
    }
    ast_free(smallcpp_ast);
    smallcpp_ast = NULL;
    if (status != 0 || error_count != 0)
        return 1;
    return 0;
}
