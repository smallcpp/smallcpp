#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
    llvm::LLVMContext context;
    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    llvm::Function *answer;

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s module.bc\n", argv[0]);
        return 2;
    }
    auto buffer = llvm::MemoryBuffer::getFile(argv[1]);
    if (!buffer) {
        std::fprintf(stderr, "%s: %s\n", argv[1], buffer.getError().message().c_str());
        return 1;
    }
    auto parsed = llvm::parseBitcodeFile(buffer.get()->getMemBufferRef(), context);
    if (!parsed) {
        std::fprintf(stderr, "%s: bitcode did not load\n", argv[1]);
        return 1;
    }
    if (llvm::verifyModule(*parsed.get(), &errors)) {
        errors.flush();
        std::fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }
    answer = parsed.get()->getFunction("answer");
    if (answer == NULL || answer->empty() || !answer->getReturnType()->isIntegerTy(32)) {
        std::fprintf(stderr, "module has no int function answer\n");
        return 1;
    }
    return 0;
}
