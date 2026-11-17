#ifndef SMALLCPP_DUMP_H
#define SMALLCPP_DUMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ast.h"

/* Structured text, one node per line. Returns 0 on success. */
int ast_write_text(const AstNode *root, const char *path);

/* Graphviz DOT. Returns 0 on success. */
int ast_write_dot(const AstNode *root, const char *path);

#ifdef __cplusplus
}
#endif

#endif
