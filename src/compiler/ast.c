#include "ast.h"

#include <stdlib.h>
#include <string.h>

AstNode *smallcpp_ast;

AstNode *ast_make(AstKind kind, char *text, AstNode *a, AstNode *b, AstNode *c, AstNode *d)
{
    AstNode *node = calloc(1, sizeof *node);
    if (node == NULL)
        return NULL;
    node->kind = kind;
    node->text = text;
    node->a = a;
    node->b = b;
    node->c = c;
    node->d = d;
    return node;
}

AstNode *ast_text(AstKind kind, char *owned)
{
    return ast_make(kind, owned, NULL, NULL, NULL, NULL);
}

AstNode *ast_named(AstKind kind, const char *spelling)
{
    return ast_make(kind, spelling != NULL ? strdup(spelling) : NULL,
                    NULL, NULL, NULL, NULL);
}

AstNode *ast_binary(AstNode *lhs, const char *op, AstNode *rhs)
{
    return ast_make(AST_BINARY, op != NULL ? strdup(op) : NULL, lhs, rhs, NULL, NULL);
}

AstNode *ast_unary(const char *op, AstNode *expr)
{
    return ast_make(AST_UNARY, op != NULL ? strdup(op) : NULL, expr, NULL, NULL, NULL);
}

AstNode *ast_assign(AstNode *lhs, const char *op, AstNode *rhs)
{
    return ast_make(AST_ASSIGN, op != NULL ? strdup(op) : NULL, lhs, rhs, NULL, NULL);
}

void ast_append(AstNode *head, AstNode *item)
{
    if (head == NULL || item == NULL)
        return;
    while (head->next != NULL)
        head = head->next;
    head->next = item;
}

void ast_add(AstNode *parent, AstNode *child)
{
    if (parent == NULL || child == NULL)
        return;
    if (parent->a == NULL)
        parent->a = child;
    else
        ast_append(parent->a, child);
}

void ast_free(AstNode *node)
{
    if (node == NULL)
        return;
    ast_free(node->a);
    ast_free(node->b);
    ast_free(node->c);
    ast_free(node->d);
    ast_free(node->next);
    free(node->text);
    free(node);
}

int ast_contains(const AstNode *node, AstKind kind)
{
    if (node == NULL)
        return 0;
    if (node->kind == kind)
        return 1;
    return ast_contains(node->a, kind)
        || ast_contains(node->b, kind)
        || ast_contains(node->c, kind)
        || ast_contains(node->d, kind)
        || ast_contains(node->next, kind);
}

const char *ast_kind_name(AstKind kind)
{
    switch (kind) {
    case AST_TRANSLATION_UNIT: return "translation-unit";
    case AST_NAMESPACE: return "namespace";
    case AST_CLASS: return "class";
    case AST_BASE: return "base";
    case AST_ACCESS: return "access";
    case AST_METHOD: return "method";
    case AST_DECLARATION: return "declaration";
    case AST_SPECIFIER: return "specifier";
    case AST_STRUCT: return "struct";
    case AST_UNION: return "union";
    case AST_ENUM: return "enum";
    case AST_ENUMERATOR: return "enumerator";
    case AST_DECLARATOR: return "declarator";
    case AST_IDENT: return "identifier";
    case AST_TYPE_NAME: return "type-name";
    case AST_QUALIFIED: return "qualified";
    case AST_POINTER: return "pointer";
    case AST_ARRAY: return "array";
    case AST_FUNCTION_DECL: return "function-declarator";
    case AST_PARAMETER: return "parameter";
    case AST_ELLIPSIS: return "ellipsis";
    case AST_INIT: return "initializer";
    case AST_COMPOUND: return "compound";
    case AST_IF: return "if";
    case AST_SWITCH: return "switch";
    case AST_WHILE: return "while";
    case AST_DO: return "do";
    case AST_FOR: return "for";
    case AST_GOTO: return "goto";
    case AST_CONTINUE: return "continue";
    case AST_BREAK: return "break";
    case AST_RETURN: return "return";
    case AST_LABEL: return "label";
    case AST_CASE: return "case";
    case AST_DEFAULT: return "default";
    case AST_EXPR_STMT: return "expression-statement";
    case AST_TRY: return "try";
    case AST_CATCH: return "catch";
    case AST_THROW: return "throw";
    case AST_NEW: return "new";
    case AST_THIS: return "this";
    case AST_LITERAL: return "literal";
    case AST_CALL: return "call";
    case AST_INDEX: return "index";
    case AST_MEMBER: return "member";
    case AST_ARROW: return "arrow";
    case AST_UNARY: return "unary";
    case AST_BINARY: return "binary";
    case AST_ASSIGN: return "assign";
    case AST_CONDITIONAL: return "conditional";
    case AST_CAST: return "cast";
    case AST_SIZEOF: return "sizeof";
    }
    return "unknown";
}
