#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower scalar functions and calls to an LLVM module, verify it, and write
 * bitcode. Covers scalar types, parameters, locals, file-scope scalars,
 * operators, control flow, calls, overloads, default arguments, varargs,
 * extern, static, inline, typedef, register, and string literals.
 * Aggregates, namespaces, and classes are rejected. print and format stay
 * builtins and are not lowered.
 * Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
