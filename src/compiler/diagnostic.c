#include "diagnostic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *source_path;
static char **source_lines;
static int source_count;

static void clear_source(void)
{
    int i;

    free(source_path);
    source_path = NULL;
    for (i = 0; i < source_count; i++)
        free(source_lines[i]);
    free(source_lines);
    source_lines = NULL;
    source_count = 0;
}

static char *read_source_line(FILE *in)
{
    size_t cap = 128;
    size_t length = 0;
    char *text = malloc(cap);
    int ch;

    if (text == NULL)
        return NULL;
    while ((ch = fgetc(in)) != EOF && ch != '\n') {
        if (ch == '\r')
            continue;
        if (length + 1 >= cap) {
            size_t next = cap * 2;
            char *grown = realloc(text, next);
            if (grown == NULL) {
                free(text);
                return NULL;
            }
            text = grown;
            cap = next;
        }
        text[length++] = (char) ch;
    }
    if (ch == EOF && length == 0) {
        free(text);
        return NULL;
    }
    text[length] = '\0';
    return text;
}

void smallcpp_diag_load(const char *path)
{
    FILE *in;
    char *line;

    clear_source();
    if (path == NULL)
        return;
    source_path = strdup(path);
    in = fopen(path, "r");
    if (in == NULL)
        return;
    while ((line = read_source_line(in)) != NULL) {
        char **grown = realloc(source_lines, (size_t) (source_count + 1) * sizeof *grown);
        if (grown == NULL) {
            free(line);
            break;
        }
        source_lines = grown;
        source_lines[source_count++] = line;
    }
    fclose(in);
}

void smallcpp_diag_report(int line, int column, const char *message)
{
    if (line < 1)
        line = 1;
    if (column < 1)
        column = 1;
    fprintf(stderr, "%s:%d:%d: %s\n",
            source_path != NULL ? source_path : "<input>",
            line, column, message != NULL ? message : "");
    if (line <= source_count)
        fprintf(stderr, "%s\n", source_lines[line - 1]);
}
