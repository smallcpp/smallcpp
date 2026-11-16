#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int load_module(const char *path, llvm::LLVMContext &context,
                       std::unique_ptr<llvm::Module> &module)
{
    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    auto buffer = llvm::MemoryBuffer::getFile(path);

    if (!buffer) {
        std::fprintf(stderr, "%s: %s\n", path, buffer.getError().message().c_str());
        return 1;
    }
    auto parsed = llvm::parseBitcodeFile(buffer.get()->getMemBufferRef(), context);
    if (!parsed) {
        std::fprintf(stderr, "%s: bitcode did not load\n", path);
        return 1;
    }
    if (llvm::verifyModule(*parsed.get(), &errors)) {
        errors.flush();
        std::fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }
    module = std::move(parsed.get());
    return 0;
}

static int check_return(llvm::Module &module)
{
    llvm::Function *answer = module.getFunction("answer");

    if (answer == NULL || answer->empty() || !answer->getReturnType()->isIntegerTy(32)) {
        std::fprintf(stderr, "module has no int function answer\n");
        return 1;
    }
    return 0;
}

static bool has_conditional_branch(const llvm::Function &function)
{
    for (const llvm::BasicBlock &block : function) {
        for (const llvm::Instruction &inst : block) {
            const auto *branch = llvm::dyn_cast<llvm::BranchInst>(&inst);
            if (branch != NULL && branch->isConditional())
                return true;
        }
    }
    return false;
}

static bool has_back_edge(llvm::Function &function)
{
    llvm::DominatorTree tree(function);

    for (llvm::BasicBlock &block : function) {
        for (llvm::BasicBlock *succ : llvm::successors(&block)) {
            if (tree.dominates(succ, &block))
                return true;
        }
    }
    return false;
}

static int check_scalars(llvm::Module &module)
{
    llvm::Function *scale = module.getFunction("scale");

    if (scale == NULL || scale->empty()) {
        std::fprintf(stderr, "module has no function scale\n");
        return 1;
    }
    if (!scale->getReturnType()->isIntegerTy(32) || scale->arg_size() != 1
        || !scale->getArg(0)->getType()->isIntegerTy(32)) {
        std::fprintf(stderr, "scale is not int(int)\n");
        return 1;
    }
    if (!has_conditional_branch(*scale)) {
        std::fprintf(stderr, "scale has no conditional branch\n");
        return 1;
    }
    if (!has_back_edge(*scale)) {
        std::fprintf(stderr, "scale has no back-edge\n");
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;

    if (argc != 3) {
        std::fprintf(stderr, "usage: %s return|scalars module.bc\n", argv[0]);
        return 2;
    }
    if (load_module(argv[2], context, module) != 0)
        return 1;
    if (std::strcmp(argv[1], "return") == 0)
        return check_return(*module);
    if (std::strcmp(argv[1], "scalars") == 0)
        return check_scalars(*module);
    std::fprintf(stderr, "usage: %s return|scalars module.bc\n", argv[0]);
    return 2;
}
