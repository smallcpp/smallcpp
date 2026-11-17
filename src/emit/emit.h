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

/* Write LLVM bitcode. debug is nonzero to record source locations. Returns 0 on success. */
int smallcpp_emit_bitcode(const AstNode *root, const char *path, int debug, const char *source);

/* Write a native object and do not link. debug is nonzero to record source locations. */
int smallcpp_emit_object(const AstNode *root, const char *path, int debug, const char *source);

/* Link an executable with the runtime. debug is nonzero to record source locations
   and to pass -g to Clang. source is the .scpp file named in that debug info. */
int smallcpp_emit_executable(const AstNode *root, const char *path, int debug, const char *source);

#ifdef __cplusplus
}
#endif

#endif
