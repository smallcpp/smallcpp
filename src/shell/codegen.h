#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower one function to an LLVM module, verify it, and write bitcode.
 * The function returns an int literal and has no parameters.
 * print and format stay builtins for a later C runtime and are not lowered.
 * Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
