#pragma once

namespace llvm {
class Module;
}

/* Compile a verified module to an object and link it into an executable. */
int emit_and_link(llvm::Module &module, const char *executable);
