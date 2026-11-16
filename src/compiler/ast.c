#include "ast.h"

#include <stdio.h>
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

AstNode *ast_catch_all(AstNode *body)
{
    return ast_make(AST_CATCH, strdup("..."), NULL, body, NULL, NULL);
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

static int format_fail(char *message, size_t message_size, const char *text)
{
    if (message != NULL && message_size > 0)
        snprintf(message, message_size, "%s", text);
    return 0;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

/* The scanned token includes the quotes. The result is the decoded text. */
static int decode_string(const char *literal, char **out, char *message, size_t message_size)
{
    const char *p = literal;
    char *buf;
    size_t n = 0;

    if (p != NULL && *p == 'L')
        p++;
    if (p == NULL || *p != '"')
        return format_fail(message, message_size, "print and format require a string literal");
    p++;
    buf = malloc(strlen(p) + 1);
    if (buf == NULL)
        return format_fail(message, message_size, "out of memory");
    while (*p != '\0' && *p != '"') {
        if (*p != '\\') {
            buf[n++] = *p++;
            continue;
        }
        p++;
        if (*p == '\0') {
            free(buf);
            return format_fail(message, message_size, "invalid format string");
        }
        if (*p >= '0' && *p <= '7') {
            int value = 0;
            int digits = 0;

            while (digits < 3 && *p >= '0' && *p <= '7') {
                value = value * 8 + (*p - '0');
                p++;
                digits++;
            }
            buf[n++] = (char)value;
            continue;
        }
        if (*p == 'x') {
            int value = 0;
            int digits = 0;
            int digit;

            p++;
            while ((digit = hex_value(*p)) >= 0) {
                value = value * 16 + digit;
                p++;
                digits++;
            }
            if (digits == 0) {
                free(buf);
                return format_fail(message, message_size, "invalid format string");
            }
            buf[n++] = (char)value;
            continue;
        }
        switch (*p) {
        case 'n': buf[n++] = '\n'; break;
        case 't': buf[n++] = '\t'; break;
        case 'r': buf[n++] = '\r'; break;
        case 'a': buf[n++] = '\a'; break;
        case 'b': buf[n++] = '\b'; break;
        case 'f': buf[n++] = '\f'; break;
        case 'v': buf[n++] = '\v'; break;
        default: buf[n++] = *p; break;
        }
        p++;
    }
    buf[n] = '\0';
    *out = buf;
    return 1;
}

struct FormatCheck {
    const char *text;
    int i;
    int length;
    int mode;
    int next_auto;
    int arg_count;
    char *message;
    size_t message_size;
};

static int is_align(char c)
{
    return c == '<' || c == '>' || c == '^';
}

static int is_format_type(char c)
{
    return c != '\0' && strchr("aAbBcdeEfFgGopsxX?", c) != NULL;
}

static int note_argument(struct FormatCheck *check, int manual, int id)
{
    if (manual) {
        if (check->mode == 1)
            return format_fail(check->message, check->message_size,
                               "format string mixes automatic and manual indexing");
        check->mode = 2;
        if (id < 0 || id >= check->arg_count)
            return format_fail(check->message, check->message_size,
                               "format argument index is out of range");
        return 1;
    }
    if (check->mode == 2)
        return format_fail(check->message, check->message_size,
                           "format string mixes automatic and manual indexing");
    check->mode = 1;
    if (check->next_auto >= check->arg_count)
        return format_fail(check->message, check->message_size,
                           "not enough arguments for format string");
    check->next_auto++;
    return 1;
}

static int parse_arg_id(struct FormatCheck *check, int *id, int *present)
{
    int value = 0;

    if (check->i >= check->length
        || check->text[check->i] < '0' || check->text[check->i] > '9') {
        *present = 0;
        *id = 0;
        return 1;
    }
    while (check->i < check->length
           && check->text[check->i] >= '0' && check->text[check->i] <= '9') {
        int digit = check->text[check->i] - '0';

        if (value > 1000000)
            return format_fail(check->message, check->message_size,
                               "format argument index is out of range");
        value = value * 10 + digit;
        check->i++;
    }
    *present = 1;
    *id = value;
    return 1;
}

static int parse_nested_argument(struct FormatCheck *check)
{
    int present = 0;
    int id = 0;

    if (check->i >= check->length || check->text[check->i] != '{')
        return format_fail(check->message, check->message_size, "invalid format specifier");
    check->i++;
    if (!parse_arg_id(check, &id, &present))
        return 0;
    if (check->i >= check->length || check->text[check->i] != '}')
        return format_fail(check->message, check->message_size,
                           "unmatched '{' in format string");
    check->i++;
    return note_argument(check, present, id);
}

static int parse_format_spec(struct FormatCheck *check)
{
    const char *text = check->text;
    int i = check->i;
    int length = check->length;

    if (i + 1 < length && is_align(text[i + 1]) && text[i] != '{' && text[i] != '}')
        i += 2;
    else if (i < length && is_align(text[i]))
        i += 1;
    if (i < length && (text[i] == '+' || text[i] == '-' || text[i] == ' '))
        i++;
    if (i < length && text[i] == '#')
        i++;
    if (i < length && text[i] == '0')
        i++;
    check->i = i;
    if (check->i < length && text[check->i] == '{') {
        if (!parse_nested_argument(check))
            return 0;
    } else if (check->i < length && text[check->i] >= '1' && text[check->i] <= '9') {
        while (check->i < length && text[check->i] >= '0' && text[check->i] <= '9')
            check->i++;
    }
    if (check->i < length && text[check->i] == '.') {
        check->i++;
        if (check->i < length && text[check->i] == '{') {
            if (!parse_nested_argument(check))
                return 0;
        } else if (check->i < length && text[check->i] >= '0' && text[check->i] <= '9') {
            while (check->i < length && text[check->i] >= '0' && text[check->i] <= '9')
                check->i++;
        } else {
            return format_fail(check->message, check->message_size, "invalid format precision");
        }
    }
    if (check->i < length && text[check->i] == 'L')
        check->i++;
    if (check->i < length && is_format_type(text[check->i]))
        check->i++;
    return 1;
}

static int parse_replacement(struct FormatCheck *check)
{
    int present = 0;
    int id = 0;
    int had_spec = 0;

    check->i++;
    if (!parse_arg_id(check, &id, &present))
        return 0;
    if (check->i < check->length && check->text[check->i] == ':') {
        had_spec = 1;
        check->i++;
        if (!parse_format_spec(check))
            return 0;
    }
    if (check->i >= check->length || check->text[check->i] != '}') {
        return format_fail(check->message, check->message_size,
                           had_spec ? "invalid format specifier"
                                    : "unmatched '{' in format string");
    }
    check->i++;
    return note_argument(check, present, id);
}

static int check_decoded_format(const char *text, int arg_count, char *message, size_t message_size)
{
    struct FormatCheck check;

    check.text = text;
    check.i = 0;
    check.length = (int)strlen(text);
    check.mode = 0;
    check.next_auto = 0;
    check.arg_count = arg_count;
    check.message = message;
    check.message_size = message_size;
    while (check.i < check.length) {
        char c = check.text[check.i];

        if (c == '{') {
            if (check.i + 1 < check.length && check.text[check.i + 1] == '{') {
                check.i += 2;
                continue;
            }
            if (!parse_replacement(&check))
                return 0;
            continue;
        }
        if (c == '}') {
            if (check.i + 1 < check.length && check.text[check.i + 1] == '}') {
                check.i += 2;
                continue;
            }
            return format_fail(message, message_size, "unmatched '}' in format string");
        }
        check.i++;
    }
    return 1;
}

int ast_format_ok(const char *literal, int arg_count, char *message, size_t message_size)
{
    char *text = NULL;
    int ok;

    if (!decode_string(literal, &text, message, message_size))
        return 0;
    ok = check_decoded_format(text, arg_count, message, message_size);
    free(text);
    return ok;
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
    case AST_PRINT: return "print";
    case AST_FORMAT: return "format";
    }
    return "unknown";
}
