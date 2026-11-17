#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower a translation unit to an LLVM module, verify it, and write bitcode.
 * Covers scalars, calls, aggregates, namespaces, classes, allocation,
 * exceptions, and print and format. The collector and native linking are
 * not implemented. Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
