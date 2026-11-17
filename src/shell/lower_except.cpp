/*
 * throw, try, and catch.
 */
#include "lower.hpp"

llvm::Function *Codegen::declare_runtime(const char *name, llvm::FunctionType *type)
{
    llvm::Function *fn = module.getFunction(name);
    if (fn != NULL)
        return fn;
    return llvm::Function::Create(type, llvm::GlobalValue::ExternalLinkage, name, module);
}

llvm::Function *Codegen::personality()
{
    auto *i32 = llvm::Type::getInt32Ty(context);
    auto *i64 = llvm::Type::getInt64Ty(context);
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(i32, {i32, i32, i64, ptr, ptr}, false);
    return declare_runtime("__gxx_personality_v0", type);
}

llvm::Function *Codegen::throw_fn()
{
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), true);
    return declare_runtime("__cxa_throw", type);
}

llvm::Function *Codegen::rethrow_fn()
{
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    return declare_runtime("__cxa_rethrow", type);
}

llvm::Constant *Codegen::type_info(const Ty &ty)
{
    std::string name = "ti." + type_tag(ty);
    llvm::GlobalVariable *global = module.getGlobalVariable(name, true);
    if (global == NULL) {
        global = new llvm::GlobalVariable(
            module, llvm::Type::getInt8Ty(context), true, llvm::GlobalValue::ExternalLinkage,
            nullptr, name);
    }
    return global;
}

void Codegen::emit_throw(const AstNode *node)
{
    bool bare = node->a == NULL;
    if (bare && in_handler == 0) {
        error("bare throw is not in a handler");
        return;
    }
    if (function != NULL)
        function->setPersonalityFn(personality());
    std::vector<llvm::Value *> args;
    llvm::Function *target = bare ? rethrow_fn() : throw_fn();
    if (!bare) {
        RValue value = emit_rvalue(node->a);
        if (failed)
            return;
        args.push_back(value.v);
    }
    if (!handlers.empty()) {
        llvm::BasicBlock *normal = new_block("throw.cont");
        builder.CreateInvoke(target->getFunctionType(), target, normal, handlers.back(), args);
        builder.SetInsertPoint(normal);
        builder.CreateUnreachable();
        return;
    }
    builder.CreateCall(target->getFunctionType(), target, args);
    builder.CreateUnreachable();
}

void Codegen::emit_try(const AstNode *node)
{
    if (function != NULL)
        function->setPersonalityFn(personality());
    llvm::BasicBlock *pad = new_block("lpad");
    llvm::BasicBlock *next = new_block("after");
    handlers.push_back(pad);
    emit_compound(node->a);
    if (!failed && !terminated())
        builder.CreateBr(next);
    handlers.pop_back();
    if (failed)
        return;
    builder.SetInsertPoint(pad);
    unsigned clauses = 0;
    for (const AstNode *clause = node->b; clause != NULL; clause = clause->next)
        clauses++;
    auto *pad_ty = llvm::StructType::get(llvm::PointerType::get(context, 0),
                                          llvm::Type::getInt32Ty(context));
    llvm::LandingPadInst *landing = builder.CreateLandingPad(pad_ty, clauses, "pad");
    std::vector<llvm::BasicBlock *> bodies;
    for (const AstNode *clause = node->b; clause != NULL; clause = clause->next) {
        bodies.push_back(new_block("catch"));
        if (clause->text != NULL && std::strcmp(clause->text, "...") == 0) {
            landing->addClause(llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0)));
            continue;
        }
        if (clause->a == NULL || clause->a->kind != AST_PARAMETER) {
            error("not a handler");
            return;
        }
        Storage storage;
        Ty ty = parse_specs(clause->a->a, Where::Param, &storage);
        if (clause->a->b != NULL) {
            Declared declared = declare_one(ty, clause->a->b);
            ty = declared.ty;
        }
        if (failed)
            return;
        landing->addClause(type_info(ty));
    }
    if (bodies.size() == 1) {
        builder.CreateBr(bodies[0]);
    } else if (!bodies.empty()) {
        llvm::Value *selector = builder.CreateExtractValue(landing, 1, "selector");
        llvm::SwitchInst *sw = builder.CreateSwitch(selector, bodies.back(), bodies.size());
        for (unsigned i = 0; i + 1 < bodies.size(); i++)
            sw->addCase(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), i + 1), bodies[i]);
    }
    for (unsigned i = 0; i < bodies.size() && !failed; i++) {
        const AstNode *clause = node->b;
        for (unsigned n = 0; n < i && clause != NULL; n++)
            clause = clause->next;
        builder.SetInsertPoint(bodies[i]);
        in_handler++;
        push_scope();
        if (clause != NULL && clause->a != NULL && clause->a->kind == AST_PARAMETER
            && clause->a->b != NULL) {
            Storage storage;
            Ty ty = parse_specs(clause->a->a, Where::Local, &storage);
            Declared declared = declare_one(ty, clause->a->b);
            if (!failed && declared.name != NULL && declared.name->text != NULL) {
                llvm::AllocaInst *addr = alloca_entry(declared.ty.ir, declared.name->text);
                bind(declared.name->text, Slot{addr, declared.ty});
            }
        }
        if (clause != NULL && !failed)
            emit_compound(clause->b);
        pop_scope();
        in_handler--;
        if (!failed && !terminated())
            builder.CreateBr(next);
    }
    if (!failed)
        builder.SetInsertPoint(next);
}
