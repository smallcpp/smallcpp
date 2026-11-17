#pragma once

extern "C" {
#include "ast.h"
}

/*
 * Lower a translation unit to an LLVM module, verify it, and write bitcode.
 * Covers scalars, calls, aggregates, namespaces, classes, allocation,
 * exceptions, and print and format. The runtime collector implements
 * smallcpp_alloc and traces stack maps. This step does not write an
 * executable. Returns 0 on success.
 */
int compile_to_bitcode(const AstNode *root, const char *bitcode_path);
