/*
 * print and format.
 */
#include "lower.hpp"

RValue Codegen::emit_native(const AstNode *node)
{
    bool formatting = node->kind == AST_FORMAT;
    llvm::FunctionType *type = llvm::FunctionType::get(
        formatting ? llvm::PointerType::get(context, 0) : llvm::Type::getVoidTy(context), true);
    llvm::Function *fn = declare_runtime(formatting ? "smallcpp_format" : "smallcpp_print", type);
    RValue text = emit_string(node->text);
    if (failed)
        return fallback();
    std::vector<llvm::Value *> args;
    args.push_back(text.v);
    for (const AstNode *arg = node->a; arg != NULL && !failed; arg = arg->next) {
        RValue value = emit_rvalue(arg);
        if (failed)
            return fallback();
        args.push_back(value.v);
    }
    llvm::Value *call = builder.CreateCall(fn->getFunctionType(), fn, args, formatting ? "formatted" : "");
    if (formatting) {
        std::vector<llvm::Value *> mapped = live_pointers();
        mapped.push_back(call);
        llvm::Function *stackmap = llvm::Intrinsic::getOrInsertDeclaration(
            &module, llvm::Intrinsic::experimental_stackmap);
        std::vector<llvm::Value *> map_args;
        map_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 1));
        map_args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
        map_args.insert(map_args.end(), mapped.begin(), mapped.end());
        builder.CreateCall(stackmap, map_args);
    }
    RValue out;
    out.ty = formatting ? string_ty() : void_ty();
    out.v = formatting ? call : nullptr;
    return out;
}
