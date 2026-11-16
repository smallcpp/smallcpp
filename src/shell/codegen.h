#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower scalar functions to an LLVM module, verify it, and write bitcode.
 * Covers scalar types, parameters, locals, file-scope scalars, operators,
 * and the control-flow statements. Calls, aggregates, namespaces, and
 * classes are rejected. print and format stay builtins and are not lowered.
 * Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
