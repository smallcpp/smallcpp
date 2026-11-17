#ifndef SMALLCPP_EMIT_H
#define SMALLCPP_EMIT_H

/*
 * The one header shared by the C frontend and the C++ IR emitter.
 * The frontend passes the in-memory syntax tree. The emitter lowers it.
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "ast.h"

/* A path ending in .bc is bitcode. Returns 0 on success. */
int smallcpp_emit_bitcode(const AstNode *root, const char *path);

/* Any other path is an executable linked with the runtime. Returns 0 on success. */
int smallcpp_emit_executable(const AstNode *root, const char *path);

#ifdef __cplusplus
}
#endif

#endif
