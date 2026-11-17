#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower a translation unit to an LLVM module and verify it. A path ending in
 * .bc is bitcode. Any other path is an executable linked with the Clang
 * runtime, the collector, and print and format. Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
int compile_to_executable(const AstNode *root, const char *path);
