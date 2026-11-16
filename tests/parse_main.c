#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yyparse(void);
extern FILE *yyin;
extern int yylineno;

static const char *input_path;

void yyerror(const char *message)
{
    fprintf(stderr, "%s:%d: %s\n", input_path, yylineno, message);
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
    return 0;
}
