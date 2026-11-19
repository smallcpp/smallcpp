#include "symbols.h"
#include "ast.h"

#include "smallcpp.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern FILE *yyin;
extern int yylineno;
extern int yycolumn;
extern int yyparse(void);
extern void yyrestart(FILE *input);
extern int yylex_destroy(void);
extern void smallcpp_reset_lexer(void);
extern void smallcpp_reset_parser(void);

enum {
    SYMBOL_USE = 0,
    SYMBOL_FUNCTION = 1,
    SYMBOL_METHOD = 2,
    SYMBOL_CLASS = 3,
    SYMBOL_VARIABLE = 4,
    SYMBOL_PARAMETER = 5,
    SYMBOL_FIELD = 6,
    SYMBOL_ENUMERATOR = 7,
    SYMBOL_NAMESPACE = 8,
    SYMBOL_TAG = 9
};

struct ScppSymbol {
    char *name;
    int line;
    int column;
    int length;
    int kind;
    int defining;
};

static struct ScppSymbol *items;
static int count;
static int cap;

static void clear_symbols(void)
{
    int i;

    for (i = 0; i < count; i++)
        free(items[i].name);
    free(items);
    items = NULL;
    count = 0;
    cap = 0;
}

static void push(const char *name, int line, int column,
                 int length, int kind, int defining)
{
    struct ScppSymbol *grown;
    int next;

    if (name == NULL || name[0] == '\0')
        return;
    if (count == cap) {
        next = cap == 0 ? 32 : cap * 2;
        grown = realloc(items, (size_t)next * sizeof *grown);
        if (grown == NULL)
            return;
        items = grown;
        cap = next;
    }
    items[count].name = strdup(name);
    if (items[count].name == NULL)
        return;
    if (line < 1)
        line = 1;
    if (column < 1)
        column = 1;
    items[count].line = line;
    items[count].column = column;
    items[count].length = length;
    items[count].kind = kind;
    items[count].defining = defining;
    count++;
}

static void add_text(const char *name, int line, int column,
                     int kind, int defining)
{
    if (name == NULL)
        return;
    push(name, line, column, (int)strlen(name), kind, defining);
}

static void add_ident(const AstNode *node, int kind, int defining)
{
    if (node == NULL)
        return;
    if (node->kind != AST_IDENT && node->kind != AST_TYPE_NAME)
        return;
    add_text(node->text, node->line, node->column, kind, defining);
}

static const AstNode *name_node(const AstNode *node)
{
    const AstNode *found;

    if (node == NULL)
        return NULL;
    if (node->kind == AST_IDENT || node->kind == AST_TYPE_NAME)
        return node;
    if (node->kind == AST_QUALIFIED)
        return name_node(node->b);
    if (node->kind == AST_INIT || node->kind == AST_BITFIELD)
        return name_node(node->a);
    if (node->kind == AST_DECLARATOR) {
        found = name_node(node->b);
        if (found != NULL)
            return found;
        return name_node(node->a);
    }
    if (node->kind == AST_FUNCTION_DECL || node->kind == AST_ARRAY
            || node->kind == AST_POINTER)
        return name_node(node->a);
    return NULL;
}

static int declarator_qualified(const AstNode *node)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_QUALIFIED)
        return 1;
    if (node->kind == AST_INIT || node->kind == AST_BITFIELD)
        return declarator_qualified(node->a);
    if (node->kind == AST_DECLARATOR)
        return declarator_qualified(node->a)
            || declarator_qualified(node->b);
    if (node->kind == AST_FUNCTION_DECL || node->kind == AST_ARRAY
            || node->kind == AST_POINTER)
        return declarator_qualified(node->a);
    return 0;
}

static const AstNode *function_decl(const AstNode *node)
{
    const AstNode *found;

    if (node == NULL)
        return NULL;
    if (node->kind == AST_FUNCTION_DECL)
        return node;
    if (node->kind == AST_INIT)
        return function_decl(node->a);
    if (node->kind == AST_DECLARATOR) {
        found = function_decl(node->b);
        if (found != NULL)
            return found;
        return function_decl(node->a);
    }
    if (node->kind == AST_POINTER || node->kind == AST_ARRAY)
        return function_decl(node->a);
    return NULL;
}

static void add_prefix(const AstNode *node)
{
    if (node == NULL)
        return;
    if (node->kind == AST_QUALIFIED) {
        if (node->a != NULL
                && (node->a->kind == AST_IDENT
                    || node->a->kind == AST_TYPE_NAME))
            add_ident(node->a, SYMBOL_USE, 0);
        add_prefix(node->b);
        return;
    }
    if (node->kind == AST_DECLARATOR || node->kind == AST_INIT
            || node->kind == AST_BITFIELD) {
        add_prefix(node->a);
        add_prefix(node->b);
        return;
    }
    if (node->kind == AST_FUNCTION_DECL || node->kind == AST_ARRAY
            || node->kind == AST_POINTER)
        add_prefix(node->a);
}

static void walk(const AstNode *node, int in_class,
                 int in_function, int in_aggregate);

static void walk_params(const AstNode *node, int in_class)
{
    for (; node != NULL; node = node->next) {
        const AstNode *name;

        if (node->kind != AST_PARAMETER)
            continue;
        name = name_node(node->b);
        add_prefix(node->b);
        add_ident(name, SYMBOL_PARAMETER, 1);
        walk(node->a, in_class, 1, 0);
        walk(node->c, in_class, 1, 0);
    }
}

static void walk_signature(const AstNode *node, int in_class)
{
    const AstNode *fn = function_decl(node);

    if (fn != NULL)
        walk_params(fn->b, in_class);
}

static void walk_declarators(const AstNode *node, int kind,
                             int in_class, int in_function)
{
    for (; node != NULL; node = node->next) {
        const AstNode *item = node;
        const AstNode *init = NULL;
        const AstNode *name;
        int item_kind = kind;

        if (item->kind == AST_INIT) {
            init = item->b;
            item = item->a;
        }
        if (function_decl(item) != NULL) {
            if (in_class && !in_function)
                item_kind = SYMBOL_METHOD;
            else
                item_kind = SYMBOL_FUNCTION;
        }
        name = name_node(item);
        add_prefix(item);
        add_ident(name, item_kind, 1);
        walk_signature(item, in_class);
        if (init != NULL)
            walk(init, in_class, in_function, 0);
    }
}

static void walk(const AstNode *node, int in_class,
                 int in_function, int in_aggregate)
{
    int kind;

    if (node == NULL)
        return;
    switch (node->kind) {
    case AST_METHOD:
        kind = SYMBOL_FUNCTION;
        if (in_class || declarator_qualified(node->b))
            kind = SYMBOL_METHOD;
        add_prefix(node->b);
        add_ident(name_node(node->b), kind, 1);
        walk_signature(node->b, in_class);
        walk(node->a, in_class, in_function, 0);
        walk(node->c, in_class, 1, 0);
        break;
    case AST_CLASS:
        add_ident(node->a, SYMBOL_CLASS, 1);
        walk(node->b, in_class, in_function, 0);
        walk(node->c, 1, 0, 0);
        break;
    case AST_NAMESPACE:
        add_text(node->text, node->line, node->column,
                 SYMBOL_NAMESPACE, 1);
        walk(node->a, 0, 0, 0);
        break;
    case AST_STRUCT:
    case AST_UNION:
        add_text(node->text, node->line, node->column,
                 SYMBOL_TAG, 1);
        walk(node->a, in_class, in_function, 1);
        break;
    case AST_ENUM:
        add_text(node->text, node->line, node->column,
                 SYMBOL_TAG, 1);
        walk(node->a, in_class, in_function, 0);
        break;
    case AST_ENUMERATOR:
        add_text(node->text, node->line, node->column,
                 SYMBOL_ENUMERATOR, 1);
        walk(node->a, in_class, in_function, 0);
        break;
    case AST_CONSTRUCTOR:
        add_text(node->text, node->line, node->column,
                 SYMBOL_METHOD, 1);
        walk_params(node->a, in_class);
        walk(node->b, in_class, 1, 0);
        walk(node->c, in_class, 1, 0);
        break;
    case AST_DESTRUCTOR:
        add_text(node->text, node->line, node->column,
                 SYMBOL_METHOD, 1);
        walk(node->a, in_class, 1, 0);
        walk_params(node->c, in_class);
        break;
    case AST_MEM_INIT:
        add_text(node->text, node->line, node->column,
                 SYMBOL_USE, 0);
        walk(node->a, in_class, in_function, 0);
        break;
    case AST_DECLARATION:
        kind = SYMBOL_VARIABLE;
        if (in_aggregate || (in_class && !in_function))
            kind = SYMBOL_FIELD;
        walk(node->a, in_class, in_function, 0);
        walk_declarators(node->b, kind, in_class, in_function);
        break;
    case AST_IDENT:
    case AST_TYPE_NAME:
        add_ident(node, SYMBOL_USE, 0);
        break;
    default:
        walk(node->a, in_class, in_function, in_aggregate);
        walk(node->b, in_class, in_function, in_aggregate);
        walk(node->c, in_class, in_function, in_aggregate);
        walk(node->d, in_class, in_function, in_aggregate);
        break;
    }
    walk(node->next, in_class, in_function, in_aggregate);
}

static char *read_file(const char *path, size_t *out_n)
{
    FILE *in;
    char *buf;
    long size;

    in = fopen(path, "rb");
    if (in == NULL)
        return NULL;
    if (fseek(in, 0, SEEK_END) != 0) {
        fclose(in);
        return NULL;
    }
    size = ftell(in);
    if (size < 0 || fseek(in, 0, SEEK_SET) != 0) {
        fclose(in);
        return NULL;
    }
    buf = malloc((size_t)size + 1);
    if (buf == NULL) {
        fclose(in);
        return NULL;
    }
    if (size > 0 && fread(buf, 1, (size_t)size, in) != (size_t)size) {
        free(buf);
        fclose(in);
        return NULL;
    }
    buf[size] = '\0';
    fclose(in);
    *out_n = (size_t)size;
    return buf;
}

/* Keep line numbers. A directive line becomes spaces. */
static char *blank_directives(const char *text, size_t n)
{
    char *out;
    size_t i = 0;
    int carry = 0;

    out = malloc(n + 1);
    if (out == NULL)
        return NULL;
    while (i < n) {
        size_t start = i;
        size_t end;
        size_t k;
        int directive = 0;
        int escaped = 0;

        while (i < n && text[i] != '\n')
            i++;
        end = i;
        k = start;
        while (k < end
                && (text[k] == ' ' || text[k] == '\t'
                    || text[k] == '\r'))
            k++;
        if (carry || (k < end && text[k] == '#'))
            directive = 1;
        if (end > start && text[end - 1] == '\\')
            escaped = 1;
        else if (end > start + 1 && text[end - 1] == '\r'
                && text[end - 2] == '\\')
            escaped = 1;
        if (directive) {
            for (k = start; k < end; k++)
                out[k] = ' ';
        } else {
            memcpy(out + start, text + start, end - start);
        }
        if (i < n && text[i] == '\n') {
            out[i] = '\n';
            i++;
        }
        carry = directive && escaped;
    }
    out[i] = '\0';
    return out;
}

static int parse_path(const char *path)
{
    FILE *in;

    in = fopen(path, "r");
    if (in == NULL)
        return -1;
    yyin = in;
    yyrestart(in);
    yylineno = 1;
    yycolumn = 1;
    yylloc.first_line = 1;
    yylloc.first_column = 1;
    yylloc.last_line = 1;
    yylloc.last_column = 1;
    smallcpp_reset_lexer();
    smallcpp_reset_parser();
    smallcpp_ast = NULL;
    yyparse();
    yylex_destroy();
    fclose(in);
    yyin = NULL;
    if (smallcpp_ast != NULL)
        return 0;
    return 1;
}

int scpp_index_file(const char *path)
{
    char *text;
    char *blanked;
    size_t n = 0;
    char pattern[] = "/tmp/scpp-index-XXXXXX";
    int fd;
    FILE *out;
    int parsed;

    clear_symbols();
    if (path == NULL)
        return -1;
    text = read_file(path, &n);
    if (text == NULL)
        return -1;
    blanked = blank_directives(text, n);
    free(text);
    if (blanked == NULL)
        return -1;
    fd = mkstemp(pattern);
    if (fd < 0) {
        free(blanked);
        return -1;
    }
    out = fdopen(fd, "w");
    if (out == NULL) {
        close(fd);
        unlink(pattern);
        free(blanked);
        return -1;
    }
    if (n > 0 && fwrite(blanked, 1, n, out) != n) {
        free(blanked);
        fclose(out);
        unlink(pattern);
        return -1;
    }
    free(blanked);
    if (fclose(out) != 0) {
        unlink(pattern);
        return -1;
    }
    parsed = parse_path(pattern);
    unlink(pattern);
    if (parsed < 0)
        return -1;
    if (smallcpp_ast != NULL) {
        walk(smallcpp_ast, 0, 0, 0);
        ast_free(smallcpp_ast);
        smallcpp_ast = NULL;
        return 0;
    }
    return 1;
}

static int in_range(int index)
{
    return index >= 0 && index < count;
}

int scpp_symbol_count(void)
{
    return count;
}

const char *scpp_symbol_name(int index)
{
    if (!in_range(index))
        return NULL;
    return items[index].name;
}

int scpp_symbol_line(int index)
{
    if (!in_range(index))
        return 0;
    return items[index].line;
}

int scpp_symbol_column(int index)
{
    if (!in_range(index))
        return 0;
    return items[index].column;
}

int scpp_symbol_length(int index)
{
    if (!in_range(index))
        return 0;
    return items[index].length;
}

int scpp_symbol_kind(int index)
{
    if (!in_range(index))
        return 0;
    return items[index].kind;
}

int scpp_symbol_defining(int index)
{
    if (!in_range(index))
        return 0;
    return items[index].defining;
}
