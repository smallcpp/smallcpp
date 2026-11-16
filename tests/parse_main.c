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

static int require_kinds(const AstNode *root, const AstKind *kinds, int count)
{
    int i;
    int ok = 1;

    for (i = 0; i < count; i++)
        ok = require_kind(root, kinds[i]) && ok;
    return ok;
}

static int expect_node(const AstNode *node, AstKind kind, const char *text, const char *where)
{
    if (node == NULL) {
        fprintf(stderr, "tree shape: missing %s at %s\n", ast_kind_name(kind), where);
        return 0;
    }
    if (node->kind != kind) {
        fprintf(stderr, "tree shape: %s is %s, expected %s\n",
                where, ast_kind_name(node->kind), ast_kind_name(kind));
        return 0;
    }
    if (text != NULL && (node->text == NULL || strcmp(node->text, text) != 0)) {
        fprintf(stderr, "tree shape: %s text is \"%s\", expected \"%s\"\n",
                where, node->text != NULL ? node->text : "", text);
        return 0;
    }
    return 1;
}

/* tree-shape.scpp: namespace, derived class, try/catch, variadic, print, format. */
static int tree_shape_ok(const AstNode *root)
{
    const AstNode *ns;
    const AstNode *point_decl;
    const AstNode *point;
    const AstNode *derived_decl;
    const AstNode *derived;
    const AstNode *base;
    const AstNode *access;
    const AstNode *move;
    const AstNode *move_decl;
    const AstNode *try_node;
    const AstNode *catch_node;
    const AstNode *report;
    const AstNode *report_decl;
    const AstNode *body;
    const AstNode *line;
    const AstNode *formatted;
    const AstNode *printed;
    int ok = 1;

    if (!expect_node(root, AST_TRANSLATION_UNIT, NULL, "root"))
        return 0;
    ns = root->a;
    ok = expect_node(ns, AST_NAMESPACE, "audio", "namespace") && ok;
    point_decl = ns != NULL ? ns->a : NULL;
    ok = expect_node(point_decl, AST_DECLARATION, NULL, "Point declaration") && ok;
    point = point_decl != NULL ? point_decl->a : NULL;
    ok = expect_node(point, AST_CLASS, NULL, "Point") && ok;
    ok = expect_node(point != NULL ? point->a : NULL, AST_IDENT, "Point", "Point name") && ok;
    ok = expect_node(point != NULL ? point->c : NULL, AST_DECLARATION, NULL, "Point member") && ok;

    derived_decl = point_decl != NULL ? point_decl->next : NULL;
    ok = expect_node(derived_decl, AST_DECLARATION, NULL, "Derived declaration") && ok;
    derived = derived_decl != NULL ? derived_decl->a : NULL;
    ok = expect_node(derived, AST_CLASS, NULL, "Derived") && ok;
    ok = expect_node(derived != NULL ? derived->a : NULL, AST_IDENT, "Derived", "Derived name") && ok;
    base = derived != NULL ? derived->b : NULL;
    ok = expect_node(base, AST_BASE, NULL, "base") && ok;
    ok = expect_node(base != NULL ? base->a : NULL, AST_ACCESS, "public", "base access") && ok;
    ok = expect_node(base != NULL ? base->b : NULL, AST_TYPE_NAME, "Point", "base name") && ok;
    access = derived != NULL ? derived->c : NULL;
    ok = expect_node(access, AST_ACCESS, "public", "member access") && ok;
    move = access != NULL ? access->next : NULL;
    ok = expect_node(move, AST_METHOD, NULL, "move") && ok;
    move_decl = move != NULL ? move->b : NULL;
    ok = expect_node(move_decl, AST_FUNCTION_DECL, NULL, "move declarator") && ok;
    ok = expect_node(move_decl != NULL ? move_decl->a : NULL, AST_IDENT, "move", "move name") && ok;
    ok = expect_node(move_decl != NULL ? move_decl->b : NULL, AST_PARAMETER, NULL, "dx") && ok;
    try_node = move != NULL && move->c != NULL ? move->c->a : NULL;
    ok = expect_node(try_node, AST_TRY, NULL, "try") && ok;
    ok = expect_node(try_node != NULL && try_node->a != NULL ? try_node->a->a : NULL,
                     AST_THROW, NULL, "throw") && ok;
    catch_node = try_node != NULL ? try_node->b : NULL;
    ok = expect_node(catch_node, AST_CATCH, NULL, "catch") && ok;
    ok = expect_node(catch_node != NULL && catch_node->b != NULL ? catch_node->b->a : NULL,
                     AST_RETURN, NULL, "catch return") && ok;

    report = ns != NULL ? ns->next : NULL;
    ok = expect_node(report, AST_METHOD, NULL, "report") && ok;
    report_decl = report != NULL ? report->b : NULL;
    ok = expect_node(report_decl != NULL ? report_decl->a : NULL, AST_IDENT, "report", "report name") && ok;
    ok = expect_node(report_decl != NULL && report_decl->b != NULL ? report_decl->b->next : NULL,
                     AST_ELLIPSIS, "...", "ellipsis") && ok;
    body = report != NULL ? report->c : NULL;
    line = body != NULL ? body->a : NULL;
    ok = expect_node(line, AST_DECLARATION, NULL, "line") && ok;
    formatted = line != NULL ? line->next : NULL;
    ok = expect_node(formatted, AST_EXPR_STMT, NULL, "format statement") && ok;
    ok = expect_node(formatted != NULL && formatted->a != NULL ? formatted->a->b : NULL,
                     AST_FORMAT, "\"{0}\"", "format call") && ok;
    printed = formatted != NULL ? formatted->next : NULL;
    ok = expect_node(printed, AST_EXPR_STMT, NULL, "print statement") && ok;
    ok = expect_node(printed != NULL ? printed->a : NULL, AST_PRINT, "\"{}\"", "print call") && ok;
    ok = expect_node(printed != NULL ? printed->next : NULL, AST_RETURN, NULL, "report return") && ok;
    if (report != NULL && report->next != NULL) {
        fprintf(stderr, "tree shape: unexpected declaration after report\n");
        ok = 0;
    }
    return ok;
}

static int tree_is_complete(const char *path, const AstNode *root)
{
    static const AstKind namespace_kinds[] = { AST_NAMESPACE };
    static const AstKind class_kinds[] = { AST_CLASS, AST_BASE, AST_METHOD };
    static const AstKind try_kinds[] = { AST_TRY, AST_CATCH, AST_THROW };
    static const AstKind variadic_kinds[] = { AST_ELLIPSIS, AST_PARAMETER };
    static const AstKind print_kinds[] = { AST_PRINT, AST_FORMAT };
    static const AstKind accept_kinds[] = {
        AST_NAMESPACE, AST_CLASS, AST_METHOD, AST_TRY, AST_CATCH, AST_THROW, AST_NEW
    };
    int ok = 1;

    if (root == NULL) {
        fprintf(stderr, "no syntax tree\n");
        return 0;
    }
    if (ends_with(path, "tree-shape.scpp"))
        return tree_shape_ok(root);
    ok = require_kind(root, AST_TRANSLATION_UNIT);
    if (ends_with(path, "namespace.scpp"))
        return require_kinds(root, namespace_kinds, 1) && ok;
    if (ends_with(path, "class-base.scpp"))
        return require_kinds(root, class_kinds, 3) && ok;
    if (ends_with(path, "try-catch.scpp"))
        return require_kinds(root, try_kinds, 3) && ok;
    if (ends_with(path, "variadic.scpp"))
        return require_kinds(root, variadic_kinds, 2) && ok;
    if (ends_with(path, "print-format.scpp"))
        return require_kinds(root, print_kinds, 2) && ok;
    if (ends_with(path, "accept.scpp"))
        return require_kinds(root, accept_kinds, 7) && ok;
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
