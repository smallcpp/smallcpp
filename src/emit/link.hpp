#pragma once

namespace llvm {
class Module;
}

/* Write a native object file. Returns 0 on success. */
int write_object_file(llvm::Module &module, const char *path);

/* Compile a verified module to an object and link it into an executable.
   debug is nonzero to pass -g to Clang. */
int emit_and_link(llvm::Module &module, const char *executable, int debug);
