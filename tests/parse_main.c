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

    if (path_len < suffix_len || strcmp(path + path_len - suffix_len, suffix) != 0)
        return 0;
    return path_len == suffix_len || path[path_len - suffix_len - 1] == '/';
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

static void find_catches(const AstNode *node, int *typed, int *every)
{
    if (node == NULL)
        return;
    if (node->kind == AST_CATCH && node->text != NULL && strcmp(node->text, "...") == 0) {
        if (node->a == NULL)
            *every = 1;
    } else if (node->kind == AST_CATCH && node->a != NULL && node->a->kind == AST_PARAMETER) {
        *typed = 1;
    }
    find_catches(node->a, typed, every);
    find_catches(node->b, typed, every);
    find_catches(node->c, typed, every);
    find_catches(node->d, typed, every);
    find_catches(node->next, typed, every);
}

/* catch-all.scpp: catch (...) is a catch with no parameter, and a typed catch remains. */
static int catch_forms_ok(const AstNode *root)
{
    static const AstKind kinds[] = { AST_TRY, AST_CATCH, AST_THROW, AST_PARAMETER };
    int typed = 0;
    int every = 0;
    int ok;

    ok = require_kinds(root, kinds, 4);
    find_catches(root, &typed, &every);
    if (!every) {
        fprintf(stderr, "syntax tree has no catch (...)\n");
        ok = 0;
    }
    if (!typed) {
        fprintf(stderr, "syntax tree has no catch parameter\n");
        ok = 0;
    }
    return ok;
}

static void find_nulls(const AstNode *node, int *saw_nullptr, int *saw_null,
                       AstKind *kind_nullptr, AstKind *kind_null)
{
    if (node == NULL)
        return;
    if (node->text != NULL && strcmp(node->text, "nullptr") == 0) {
        *saw_nullptr = 1;
        *kind_nullptr = node->kind;
    }
    if (node->text != NULL && strcmp(node->text, "NULL") == 0) {
        *saw_null = 1;
        *kind_null = node->kind;
    }
    find_nulls(node->a, saw_nullptr, saw_null, kind_nullptr, kind_null);
    find_nulls(node->b, saw_nullptr, saw_null, kind_nullptr, kind_null);
    find_nulls(node->c, saw_nullptr, saw_null, kind_nullptr, kind_null);
    find_nulls(node->d, saw_nullptr, saw_null, kind_nullptr, kind_null);
    find_nulls(node->next, saw_nullptr, saw_null, kind_nullptr, kind_null);
}

static int nulls_match(const AstNode *root)
{
    int saw_nullptr = 0;
    int saw_null = 0;
    AstKind kind_nullptr = AST_TRANSLATION_UNIT;
    AstKind kind_null = AST_TRANSLATION_UNIT;
    int ok = require_kind(root, AST_NULL);

    find_nulls(root, &saw_nullptr, &saw_null, &kind_nullptr, &kind_null);
    if (!saw_nullptr || !saw_null) {
        fprintf(stderr, "syntax tree is missing nullptr or NULL\n");
        return 0;
    }
    if (kind_nullptr != AST_NULL || kind_null != AST_NULL || kind_nullptr != kind_null) {
        fprintf(stderr, "nullptr and NULL are not the same kind of node\n");
        ok = 0;
    }
    return ok;
}

static int new_is_pointer(const AstNode *node)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_NEW && node->a != NULL && node->a->kind == AST_POINTER)
        return 1;
    return new_is_pointer(node->a) || new_is_pointer(node->b) || new_is_pointer(node->c)
        || new_is_pointer(node->d) || new_is_pointer(node->next);
}

static int delete_is_noop(const AstNode *node)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_DELETE && node->text != NULL && strcmp(node->text, "noop") == 0
        && node->a != NULL)
        return 1;
    return delete_is_noop(node->a) || delete_is_noop(node->b) || delete_is_noop(node->c)
        || delete_is_noop(node->d) || delete_is_noop(node->next);
}

static int delete_tree_ok(const AstNode *root)
{
    int ok = require_kind(root, AST_NEW) && require_kind(root, AST_DELETE);

    if (!new_is_pointer(root)) {
        fprintf(stderr, "new does not create a pointer\n");
        ok = 0;
    }
    if (!delete_is_noop(root)) {
        fprintf(stderr, "delete is not a no-op with an operand\n");
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
    if (ends_with(path, "catch-all.scpp"))
        return catch_forms_ok(root) && ok;
    if (ends_with(path, "variadic.scpp"))
        return require_kinds(root, variadic_kinds, 2) && ok;
    if (ends_with(path, "print-format.scpp"))
        return require_kinds(root, print_kinds, 2) && ok;
    if (ends_with(path, "null.scpp"))
        return nulls_match(root) && ok;
    if (ends_with(path, "delete.scpp"))
        return delete_tree_ok(root) && ok;
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
