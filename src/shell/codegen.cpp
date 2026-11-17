/*
 * Drive lowering, then verify the module and write bitcode.
 */
#include "codegen.h"
#include "link.hpp"
#include "lower.hpp"

Codegen::Codegen(llvm::LLVMContext &context, llvm::Module &module)
    : context(context), module(module), builder(context)
{
    scopes.emplace_back();
    namespaces[""] = LookupScope{};
    module.setDataLayout(
        "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
}

int Codegen::run(const AstNode *root)
{
    if (root == NULL || root->kind != AST_TRANSLATION_UNIT) {
        error("expected a translation unit");
        return 1;
    }
    for (const AstNode *item = root->a; item != NULL; item = item->next) {
        declare_item(item);
        if (failed)
            return 1;
    }
    for (auto &group : overloads) {
        for (Func &fn : group.second) {
            if (fn.body == NULL)
                continue;
            emit_body(fn);
            if (failed)
                return 1;
        }
    }
    return 0;
}

static int lower_verified(const AstNode *root, llvm::LLVMContext &context,
                           std::unique_ptr<llvm::Module> &module)
{
    module = std::make_unique<llvm::Module>("smallcpp", context);
    Codegen codegen(context, *module);
    if (codegen.run(root) != 0)
        return 1;

    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    if (llvm::verifyModule(*module, &errors)) {
        errors.flush();
        std::fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }
    return 0;
}

int compile_to_bitcode(const AstNode *root, const char *bitcode_path)
{
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    if (lower_verified(root, context, module) != 0)
        return 1;

    std::error_code error;
    llvm::raw_fd_ostream out(bitcode_path, error, llvm::sys::fs::OF_None);
    if (error) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, error.message().c_str());
        return 1;
    }
    llvm::WriteBitcodeToFile(*module, out);
    out.flush();
    if (out.has_error()) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, out.error().message().c_str());
        return 1;
    }
    return 0;
}

int compile_to_executable(const AstNode *root, const char *path)
{
    llvm::LLVMContext context;
    std::unique_ptr<llvm::Module> module;
    if (lower_verified(root, context, module) != 0)
        return 1;
    return emit_and_link(*module, path);
}
