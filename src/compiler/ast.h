#ifndef SMALLCPP_AST_H
#define SMALLCPP_AST_H

#include <stddef.h>

typedef enum AstKind {
    AST_TRANSLATION_UNIT,
    AST_NAMESPACE,
    AST_CLASS,
    AST_BASE,
    AST_ACCESS,
    AST_METHOD,
    AST_DECLARATION,
    AST_SPECIFIER,
    AST_STRUCT,
    AST_UNION,
    AST_ENUM,
    AST_ENUMERATOR,
    AST_DECLARATOR,
    AST_IDENT,
    AST_TYPE_NAME,
    AST_QUALIFIED,
    AST_POINTER,
    AST_ARRAY,
    AST_FUNCTION_DECL,
    AST_PARAMETER,
    AST_ELLIPSIS,
    AST_INIT,
    AST_COMPOUND,
    AST_IF,
    AST_SWITCH,
    AST_WHILE,
    AST_DO,
    AST_FOR,
    AST_GOTO,
    AST_CONTINUE,
    AST_BREAK,
    AST_RETURN,
    AST_LABEL,
    AST_CASE,
    AST_DEFAULT,
    AST_EXPR_STMT,
    AST_TRY,
    AST_CATCH,
    AST_THROW,
    AST_NEW,
    AST_THIS,
    AST_LITERAL,
    AST_CALL,
    AST_INDEX,
    AST_MEMBER,
    AST_ARROW,
    AST_UNARY,
    AST_BINARY,
    AST_ASSIGN,
    AST_CONDITIONAL,
    AST_CAST,
    AST_SIZEOF,
    AST_PRINT,
    AST_FORMAT,
    AST_NULL,
    AST_DELETE,
    AST_CONSTRUCTOR,
    AST_DESTRUCTOR,
    AST_MEM_INIT,
    AST_USING,
    AST_BITFIELD,
    AST_DESIGNATED
} AstKind;

typedef struct AstNode {
    AstKind kind;
    char *text;
    struct AstNode *a;
    struct AstNode *b;
    struct AstNode *c;
    struct AstNode *d;
    struct AstNode *next;
} AstNode;

/* text is owned by the node. Children are a, b, c, and d. next links a list. */
AstNode *ast_make(AstKind kind, char *text, AstNode *a, AstNode *b, AstNode *c, AstNode *d);
AstNode *ast_text(AstKind kind, char *owned);
AstNode *ast_named(AstKind kind, const char *spelling);
AstNode *ast_binary(AstNode *lhs, const char *op, AstNode *rhs);
AstNode *ast_unary(const char *op, AstNode *expr);
AstNode *ast_assign(AstNode *lhs, const char *op, AstNode *rhs);
/* Catch every exception type. The node text is "...". There is no parameter. */
AstNode *ast_catch_all(AstNode *body);
/* nullptr and NULL are the same kind of node. spelling is the keyword used. */
AstNode *ast_null(const char *spelling);
/* new creates a pointer to type. args are constructor arguments, or NULL. */
AstNode *ast_new_expr(AstNode *type, AstNode *args);
/* delete is a no-op. The text is "noop". The operand is still required. */
AstNode *ast_delete_noop(AstNode *operand);
void ast_append(AstNode *head, AstNode *item);
void ast_add(AstNode *parent, AstNode *child);
void ast_free(AstNode *node);
int ast_contains(const AstNode *node, AstKind kind);
const char *ast_kind_name(AstKind kind);

/*
 * literal is a scanned string token, including quotes. arg_count is the
 * number of arguments after that string. Returns 1 when the literal is a
 * C++23 format string those arguments can fill. On failure writes a
 * message and returns 0. This is the format-string syntax, not formatting.
 */
int ast_format_ok(const char *literal, int arg_count, char *message, size_t message_size);

extern AstNode *smallcpp_ast;

#endif
