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

static int list_count(const AstNode *node)
{
    int count = 0;

    while (node != NULL) {
        count++;
        node = node->next;
    }
    return count;
}

static const AstNode *find_class(const AstNode *node, const char *name)
{
    const AstNode *found;

    if (node == NULL)
        return NULL;
    if (node->kind == AST_CLASS && node->a != NULL && node->a->text != NULL
        && strcmp(node->a->text, name) == 0)
        return node;
    found = find_class(node->a, name);
    if (found != NULL)
        return found;
    found = find_class(node->b, name);
    if (found != NULL)
        return found;
    found = find_class(node->c, name);
    if (found != NULL)
        return found;
    found = find_class(node->d, name);
    if (found != NULL)
        return found;
    return find_class(node->next, name);
}

static const AstNode *find_ctor_inits(const AstNode *node, const char *name, int count)
{
    const AstNode *found;

    if (node == NULL)
        return NULL;
    if (node->kind == AST_CONSTRUCTOR && node->text != NULL && strcmp(node->text, name) == 0
        && list_count(node->b) == count)
        return node;
    found = find_ctor_inits(node->a, name, count);
    if (found != NULL)
        return found;
    found = find_ctor_inits(node->b, name, count);
    if (found != NULL)
        return found;
    found = find_ctor_inits(node->c, name, count);
    if (found != NULL)
        return found;
    found = find_ctor_inits(node->d, name, count);
    if (found != NULL)
        return found;
    return find_ctor_inits(node->next, name, count);
}

static const AstNode *find_named(const AstNode *node, AstKind kind, const char *text)
{
    const AstNode *found;

    if (node == NULL)
        return NULL;
    if (node->kind == kind && node->text != NULL && strcmp(node->text, text) == 0)
        return node;
    found = find_named(node->a, kind, text);
    if (found != NULL)
        return found;
    found = find_named(node->b, kind, text);
    if (found != NULL)
        return found;
    found = find_named(node->c, kind, text);
    if (found != NULL)
        return found;
    found = find_named(node->d, kind, text);
    if (found != NULL)
        return found;
    return find_named(node->next, kind, text);
}

static int ctor_tree_ok(const AstNode *root)
{
    const AstNode *point;
    const AstNode *pair;
    int ok = require_kind(root, AST_CONSTRUCTOR) && require_kind(root, AST_DESTRUCTOR)
        && require_kind(root, AST_MEM_INIT);

    point = find_ctor_inits(root, "Point", 1);
    if (point == NULL || point->b->text == NULL || strcmp(point->b->text, "x") != 0) {
        fprintf(stderr, "Point constructor has no member initializer x\n");
        ok = 0;
    }
    if (find_named(root, AST_DESTRUCTOR, "Point") == NULL) {
        fprintf(stderr, "syntax tree has no Point destructor\n");
        ok = 0;
    }
    pair = find_ctor_inits(root, "Pair", 2);
    if (pair == NULL) {
        fprintf(stderr, "Pair constructor does not initialize x and y\n");
        ok = 0;
    }
    if (find_named(root, AST_DESTRUCTOR, "Pair") == NULL) {
        fprintf(stderr, "syntax tree has no Pair destructor\n");
        ok = 0;
    }
    return ok;
}

static int parameter_has_default(const AstNode *node, const char *literal)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_PARAMETER && node->c != NULL && node->c->kind == AST_LITERAL
        && node->c->text != NULL && strcmp(node->c->text, literal) == 0)
        return 1;
    return parameter_has_default(node->a, literal) || parameter_has_default(node->b, literal)
        || parameter_has_default(node->c, literal) || parameter_has_default(node->d, literal)
        || parameter_has_default(node->next, literal);
}

static int default_arg_ok(const AstNode *root)
{
    if (!parameter_has_default(root, "2")) {
        fprintf(stderr, "syntax tree has no parameter default 2\n");
        return 0;
    }
    return 1;
}

static void find_usings(const AstNode *node, int *directive, int *declaration)
{
    if (node == NULL)
        return;
    if (node->kind == AST_USING) {
        if (node->text != NULL && strcmp(node->text, "namespace") == 0)
            *directive = 1;
        if (node->text == NULL && node->a != NULL && node->a->kind == AST_QUALIFIED)
            *declaration = 1;
    }
    find_usings(node->a, directive, declaration);
    find_usings(node->b, directive, declaration);
    find_usings(node->c, directive, declaration);
    find_usings(node->d, directive, declaration);
    find_usings(node->next, directive, declaration);
}

static int using_ok(const AstNode *root)
{
    int directive = 0;
    int declaration = 0;
    int ok = require_kind(root, AST_USING);

    find_usings(root, &directive, &declaration);
    if (!directive) {
        fprintf(stderr, "syntax tree has no using-directive\n");
        ok = 0;
    }
    if (!declaration) {
        fprintf(stderr, "syntax tree has no using-declaration\n");
        ok = 0;
    }
    return ok;
}

static const char *method_name(const AstNode *method)
{
    const AstNode *decl;

    if (method == NULL || method->kind != AST_METHOD)
        return NULL;
    decl = method->b;
    if (decl == NULL || decl->kind != AST_FUNCTION_DECL || decl->a == NULL)
        return NULL;
    return decl->a->text;
}

static void find_overloads(const AstNode *node, int *one, int *two)
{
    const char *name;

    if (node == NULL)
        return;
    name = method_name(node);
    if (name != NULL && strcmp(name, "sum") == 0) {
        if (list_count(node->b->b) == 1)
            *one = 1;
        if (list_count(node->b->b) == 2)
            *two = 1;
    }
    find_overloads(node->a, one, two);
    find_overloads(node->b, one, two);
    find_overloads(node->c, one, two);
    find_overloads(node->d, one, two);
    find_overloads(node->next, one, two);
}

static int overload_ok(const AstNode *root)
{
    int one = 0;
    int two = 0;

    find_overloads(root, &one, &two);
    if (!one || !two) {
        fprintf(stderr, "syntax tree does not have sum overloads with 1 and 2 parameters\n");
        return 0;
    }
    return 1;
}

static int multi_base_ok(const AstNode *root)
{
    const AstNode *type_c;
    const AstNode *ctor;
    const AstNode *init;
    int saw_a = 0;
    int saw_b = 0;
    int ok = 1;

    type_c = find_class(root, "C");
    if (type_c == NULL || type_c->b == NULL || type_c->b->kind != AST_BASE
        || type_c->b->next == NULL || type_c->b->next->kind != AST_BASE) {
        fprintf(stderr, "class C does not list two bases\n");
        ok = 0;
    }
    ctor = find_named(type_c, AST_CONSTRUCTOR, "C");
    if (ctor == NULL) {
        fprintf(stderr, "class C has no constructor\n");
        return 0;
    }
    for (init = ctor->b; init != NULL; init = init->next) {
        if (init->kind != AST_MEM_INIT || init->text == NULL)
            continue;
        if (strcmp(init->text, "A") == 0)
            saw_a = 1;
        if (strcmp(init->text, "B") == 0)
            saw_b = 1;
    }
    if (!saw_a || !saw_b) {
        fprintf(stderr, "constructor does not initialize both bases\n");
        ok = 0;
    }
    return ok;
}

static int super_ok(const AstNode *root)
{
    int ok = require_kind(root, AST_SUPER) && require_kind(root, AST_THIS);

    if (find_named(root, AST_SUPER, "super") == NULL) {
        fprintf(stderr, "syntax tree has no super keyword\n");
        ok = 0;
    }
    return ok;
}

static int text_is(const AstNode *node, const char *text)
{
    return node != NULL && node->text != NULL && strcmp(node->text, text) == 0;
}

static const AstNode *function_id(const AstNode *decl)
{
    const AstNode *id;

    if (decl == NULL || decl->kind != AST_FUNCTION_DECL)
        return NULL;
    id = decl->a;
    while (id != NULL && id->kind == AST_QUALIFIED)
        id = id->b;
    return id;
}

static int walk_const_method(const AstNode *node)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_METHOD && text_is(function_id(node->b), "read")
        && node->b->c != NULL && node->b->c->kind == AST_SPECIFIER
        && text_is(node->b->c, "const") && node->c != NULL
        && node->c->kind == AST_COMPOUND)
        return 1;
    return walk_const_method(node->a) || walk_const_method(node->b)
        || walk_const_method(node->c) || walk_const_method(node->d)
        || walk_const_method(node->next);
}

static int const_method_ok(const AstNode *root)
{
    if (!walk_const_method(root)) {
        fprintf(stderr, "syntax tree has no const method read\n");
        return 0;
    }
    return 1;
}

static void walk_new_forms(const AstNode *node, int *array, int *pointer)
{
    if (node == NULL)
        return;
    if (node->kind == AST_NEW && node->a != NULL && node->a->kind == AST_POINTER
        && node->a->a != NULL) {
        if (node->a->a->kind == AST_ARRAY)
            *array = 1;
        if (node->a->a->kind == AST_POINTER)
            *pointer = 1;
    }
    walk_new_forms(node->a, array, pointer);
    walk_new_forms(node->b, array, pointer);
    walk_new_forms(node->c, array, pointer);
    walk_new_forms(node->d, array, pointer);
    walk_new_forms(node->next, array, pointer);
}

static int new_forms_ok(const AstNode *root)
{
    int array = 0;
    int pointer = 0;

    walk_new_forms(root, &array, &pointer);
    if (!array) {
        fprintf(stderr, "syntax tree has no array new-expression\n");
        return 0;
    }
    if (!pointer) {
        fprintf(stderr, "syntax tree has no pointer-type new-expression\n");
        return 0;
    }
    return 1;
}

static int has_specifier(const AstNode *node, const char *text)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_SPECIFIER && text_is(node, text))
        return 1;
    return has_specifier(node->a, text) || has_specifier(node->b, text)
        || has_specifier(node->c, text) || has_specifier(node->d, text)
        || has_specifier(node->next, text);
}

static int has_literal(const AstNode *node, const char *text)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_LITERAL && text_is(node, text))
        return 1;
    return has_literal(node->a, text) || has_literal(node->b, text)
        || has_literal(node->c, text) || has_literal(node->d, text)
        || has_literal(node->next, text);
}

static int bool_ok(const AstNode *root)
{
    int ok = 1;

    if (!has_specifier(root, "bool")) {
        fprintf(stderr, "syntax tree has no bool specifier\n");
        ok = 0;
    }
    if (!has_literal(root, "true") || !has_literal(root, "false")) {
        fprintf(stderr, "syntax tree is missing true or false\n");
        ok = 0;
    }
    return ok;
}

static int walk_bit_field(const AstNode *node, const char *name, const char *width)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_BITFIELD && text_is(node->a, name)
        && node->b != NULL && node->b->kind == AST_LITERAL && text_is(node->b, width))
        return 1;
    return walk_bit_field(node->a, name, width) || walk_bit_field(node->b, name, width)
        || walk_bit_field(node->c, name, width) || walk_bit_field(node->d, name, width)
        || walk_bit_field(node->next, name, width);
}

static int bit_field_ok(const AstNode *root)
{
    int ok = require_kind(root, AST_BITFIELD);

    if (!walk_bit_field(root, "ready", "1") || !walk_bit_field(root, "code", "7")) {
        fprintf(stderr, "syntax tree is missing a named bit-field\n");
        ok = 0;
    }
    return ok;
}

static int walk_designated(const AstNode *node, int *member, int *index)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_DESIGNATED) {
        if (text_is(node, "x") && node->b != NULL && text_is(node->b, "1"))
            *member = 1;
        if (node->text == NULL && node->a != NULL && text_is(node->a, "1")
            && node->b != NULL && text_is(node->b, "4"))
            *index = 1;
    }
    return walk_designated(node->a, member, index) || walk_designated(node->b, member, index)
        || walk_designated(node->c, member, index) || walk_designated(node->d, member, index)
        || walk_designated(node->next, member, index);
}

static int designated_ok(const AstNode *root)
{
    int member = 0;
    int index = 0;

    walk_designated(root, &member, &index);
    if (!member) {
        fprintf(stderr, "syntax tree has no member designator\n");
        return 0;
    }
    if (!index) {
        fprintf(stderr, "syntax tree has no array designator\n");
        return 0;
    }
    return 1;
}

static void walk_functions(const AstNode *node, int *ready, int *take)
{
    if (node == NULL)
        return;
    if (node->kind == AST_FUNCTION_DECL) {
        if (text_is(function_id(node), "ready") && node->b == NULL)
            *ready = 1;
        if (text_is(function_id(node), "take") && node->b != NULL
            && node->b->kind == AST_PARAMETER)
            *take = 1;
    }
    walk_functions(node->a, ready, take);
    walk_functions(node->b, ready, take);
    walk_functions(node->c, ready, take);
    walk_functions(node->d, ready, take);
    walk_functions(node->next, ready, take);
}

static int empty_params_ok(const AstNode *root)
{
    int ready = 0;
    int take = 0;

    walk_functions(root, &ready, &take);
    if (!ready) {
        fprintf(stderr, "syntax tree has no empty parameter list\n");
        return 0;
    }
    if (!take) {
        fprintf(stderr, "syntax tree has no (void) parameter list\n");
        return 0;
    }
    return 1;
}

static int brace_pair(const AstNode *node, const char *first, const char *second)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_INIT && node->a != NULL && node->a->kind == AST_LITERAL
        && text_is(node->a, first) && node->a->next != NULL && text_is(node->a->next, second))
        return 1;
    return brace_pair(node->a, first, second) || brace_pair(node->b, first, second)
        || brace_pair(node->c, first, second) || brace_pair(node->d, first, second)
        || brace_pair(node->next, first, second);
}

static int brace_init_ok(const AstNode *root)
{
    int ok = require_kind(root, AST_STRUCT) && require_kind(root, AST_ARRAY);

    if (!brace_pair(root, "1", "2") || !brace_pair(root, "3", "4")) {
        fprintf(stderr, "syntax tree is missing a brace initializer\n");
        ok = 0;
    }
    return ok;
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
    if (ends_with(path, "ctor-dtor.scpp"))
        return ctor_tree_ok(root) && ok;
    if (ends_with(path, "default-arg.scpp"))
        return default_arg_ok(root) && ok;
    if (ends_with(path, "using.scpp"))
        return using_ok(root) && ok;
    if (ends_with(path, "overload.scpp"))
        return overload_ok(root) && ok;
    if (ends_with(path, "multi-base.scpp"))
        return multi_base_ok(root) && ok;
    if (ends_with(path, "super.scpp"))
        return super_ok(root) && ok;
    if (ends_with(path, "const-method.scpp"))
        return const_method_ok(root) && ok;
    if (ends_with(path, "new-array.scpp"))
        return new_forms_ok(root) && ok;
    if (ends_with(path, "bool.scpp"))
        return bool_ok(root) && ok;
    if (ends_with(path, "bit-field.scpp"))
        return bit_field_ok(root) && ok;
    if (ends_with(path, "designated.scpp"))
        return designated_ok(root) && ok;
    if (ends_with(path, "empty-params.scpp"))
        return empty_params_ok(root) && ok;
    if (ends_with(path, "brace-init.scpp"))
        return brace_init_ok(root) && ok;
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
