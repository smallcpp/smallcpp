/*
 * Compile a verified module and link the Clang runtime, the collector, and
 * print and format.
 */
#include "link.hpp"

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"

#include <cstdio>
#include <string>

static void keep_frame_pointers(llvm::Module &module)
{
    for (llvm::Function &fn : module) {
        if (!fn.isDeclaration())
            fn.addFnAttr("frame-pointer", "all");
    }
}

static int write_object(llvm::Module &module, const std::string &path)
{
    llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
    std::string error;
    const llvm::Target *target;
    llvm::TargetOptions options;
    std::unique_ptr<llvm::TargetMachine> machine;
    std::error_code code;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    module.setTargetTriple(triple);
    keep_frame_pointers(module);
    target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (target == NULL) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 1;
    }
    machine.reset(target->createTargetMachine(
        triple, llvm::sys::getHostCPUName(), "", options, llvm::Reloc::PIC_, std::nullopt,
        llvm::CodeGenOptLevel::Default));
    if (!machine) {
        std::fprintf(stderr, "no target machine\n");
        return 1;
    }
    llvm::raw_fd_ostream out(path, code, llvm::sys::fs::OF_None);
    llvm::legacy::PassManager pass;
    if (code) {
        std::fprintf(stderr, "%s: %s\n", path.c_str(), code.message().c_str());
        return 1;
    }
    if (machine->addPassesToEmitFile(pass, out, nullptr, llvm::CodeGenFileType::ObjectFile)) {
        std::fprintf(stderr, "object emission is not supported\n");
        return 1;
    }
    pass.run(module);
    out.flush();
    if (out.has_error()) {
        std::fprintf(stderr, "%s: %s\n", path.c_str(), out.error().message().c_str());
        return 1;
    }
    return 0;
}

int write_object_file(llvm::Module &module, const char *path)
{
    return write_object(module, path);
}

int emit_and_link(llvm::Module &module, const char *executable, int debug)
{
    std::string object = std::string(executable) + ".o";
    llvm::StringRef clang = SMALLCPP_CLANGXX;
    llvm::StringRef runtime = SMALLCPP_RUNTIME_LIB;
    std::string error;
    int status;
    llvm::SmallVector<llvm::StringRef, 8> args;

    if (write_object(module, object) != 0) {
        llvm::sys::fs::remove(object);
        return 1;
    }
    args.push_back(clang);
    args.push_back("-no-pie");
    if (debug)
        args.push_back("-g");
    args.push_back("-o");
    args.push_back(executable);
    args.push_back(object);
    args.push_back("-Wl,-u,smallcpp_uncaught_init");
    args.push_back(runtime);
    args.push_back("-ldl");
    status = llvm::sys::ExecuteAndWait(clang, args, std::nullopt, {}, 0, 0, &error);
    llvm::sys::fs::remove(object);
    if (status != 0) {
        if (!error.empty())
            std::fprintf(stderr, "%s\n", error.c_str());
        std::fprintf(stderr, "link failed\n");
        return 1;
    }
    return 0;
}
