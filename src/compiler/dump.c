#include "dump.h"

#include <stdio.h>

static void write_escaped(FILE *out, const char *text, int dot)
{
    const char *p;

    for (p = text; *p != '\0'; ++p) {
        if (*p == '\\' || *p == '"')
            fputc('\\', out);
        if (*p == '\n') {
            fputs("\\n", out);
            continue;
        }
        if (dot && (*p == '{' || *p == '}' || *p == '<' || *p == '>' || *p == '|'))
            fputc('\\', out);
        fputc(*p, out);
    }
}

static void write_text_node(FILE *out, const AstNode *node, int depth)
{
    int i;

    if (node == NULL)
        return;
    for (i = 0; i < depth; ++i)
        fputs("  ", out);
    fputs(ast_kind_name(node->kind), out);
    if (node->text != NULL) {
        fputc(' ', out);
        fputc('"', out);
        write_escaped(out, node->text, 0);
        fputc('"', out);
    }
    fprintf(out, " %d:%d\n", node->line, node->column);
    write_text_node(out, node->a, depth + 1);
    write_text_node(out, node->b, depth + 1);
    write_text_node(out, node->c, depth + 1);
    write_text_node(out, node->d, depth + 1);
    write_text_node(out, node->next, depth);
}

int ast_write_text(const AstNode *root, const char *path)
{
    FILE *out;

    out = fopen(path, "w");
    if (out == NULL) {
        perror(path);
        return 1;
    }
    write_text_node(out, root, 0);
    if (ferror(out) || fclose(out) != 0) {
        perror(path);
        return 1;
    }
    return 0;
}

static int write_dot_node(FILE *out, const AstNode *node, int *next_id)
{
    int id;
    int child;
    static const char *const slots[] = {"a", "b", "c", "d"};
    const AstNode *children[4];
    int slot;

    if (node == NULL)
        return -1;
    id = (*next_id)++;
    fprintf(out, "  n%d [label=\"%s", id, ast_kind_name(node->kind));
    if (node->text != NULL) {
        fputs("\\n", out);
        write_escaped(out, node->text, 1);
    }
    fputs("\"];\n", out);
    children[0] = node->a;
    children[1] = node->b;
    children[2] = node->c;
    children[3] = node->d;
    for (slot = 0; slot < 4; ++slot) {
        child = write_dot_node(out, children[slot], next_id);
        if (child >= 0)
            fprintf(out, "  n%d -> n%d [label=\"%s\"];\n", id, child, slots[slot]);
    }
    child = write_dot_node(out, node->next, next_id);
    if (child >= 0)
        fprintf(out, "  n%d -> n%d [label=\"next\"];\n", id, child);
    return id;
}

int ast_write_dot(const AstNode *root, const char *path)
{
    FILE *out;
    int next_id = 0;

    out = fopen(path, "w");
    if (out == NULL) {
        perror(path);
        return 1;
    }
    fputs("digraph ast {\n", out);
    write_dot_node(out, root, &next_id);
    fputs("}\n", out);
    if (ferror(out) || fclose(out) != 0) {
        perror(path);
        return 1;
    }
    return 0;
}
