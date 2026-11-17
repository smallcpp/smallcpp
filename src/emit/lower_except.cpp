/*
 * throw, try, and catch.
 *
 * The personality is the Clang runtime's __gxx_personality_v0. A thrown
 * value is stored in memory from __cxa_allocate_exception and passed to
 * __cxa_throw with a type_info the personality can match. A handler calls
 * __cxa_begin_catch and __cxa_end_catch. A bare throw is __cxa_rethrow.
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
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {ptr, ptr, ptr}, false);
    return declare_runtime("__cxa_throw", type);
}

llvm::Function *Codegen::rethrow_fn()
{
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    return declare_runtime("__cxa_rethrow", type);
}

llvm::Function *Codegen::allocate_exception()
{
    auto *i64 = llvm::Type::getInt64Ty(context);
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(ptr, {i64}, false);
    return declare_runtime("__cxa_allocate_exception", type);
}

llvm::Function *Codegen::begin_catch_fn()
{
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(ptr, {ptr}, false);
    return declare_runtime("__cxa_begin_catch", type);
}

llvm::Function *Codegen::end_catch_fn()
{
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), false);
    return declare_runtime("__cxa_end_catch", type);
}

llvm::Function *Codegen::typeid_fn()
{
    auto *ptr = llvm::PointerType::get(context, 0);
    return llvm::Intrinsic::getOrInsertDeclaration(
        &module, llvm::Intrinsic::eh_typeid_for, {ptr});
}

void Codegen::note_unwind()
{
    if (function == NULL)
        return;
    function->setPersonalityFn(personality());
    function->setUWTableKind(llvm::UWTableKind::Default);
}

void Codegen::close_open_catches()
{
    if (terminated())
        return;
    llvm::Function *end = end_catch_fn();
    for (int i = static_cast<int>(catch_open.size()) - 1; i >= 0; i--) {
        if (!catch_open[static_cast<unsigned>(i)])
            continue;
        builder.CreateCall(end->getFunctionType(), end, {});
        catch_open[static_cast<unsigned>(i)] = 0;
    }
}

llvm::Constant *Codegen::type_info(const Ty &ty)
{
    std::string tag = type_tag(ty);
    std::string name = "ti." + tag;
    llvm::GlobalVariable *global = module.getGlobalVariable(name, true);
    if (global != NULL)
        return global;

    auto *ptr = llvm::PointerType::get(context, 0);
    auto *info_ty = llvm::StructType::get(context, {ptr, ptr});
    llvm::Constant *bytes = llvm::ConstantDataArray::getString(context, tag, true);
    auto *name_g = new llvm::GlobalVariable(
        module, bytes->getType(), true, llvm::GlobalValue::PrivateLinkage, bytes, "ts." + tag);
    const char *vtable_name = "_ZTVN10__cxxabiv123__fundamental_type_infoE";
    llvm::GlobalVariable *vtable = module.getGlobalVariable(vtable_name, true);
    if (vtable == NULL) {
        vtable = new llvm::GlobalVariable(
            module, ptr, false, llvm::GlobalValue::ExternalLinkage, nullptr, vtable_name);
    }
    llvm::Constant *index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 2);
    llvm::Constant *vptr = llvm::ConstantExpr::getGetElementPtr(
        ptr, vtable, index, llvm::GEPNoWrapFlags::inBounds());
    auto *init = llvm::ConstantStruct::get(info_ty, {vptr, name_g});
    global = new llvm::GlobalVariable(
        module, info_ty, true, llvm::GlobalValue::PrivateLinkage, init, name);
    return global;
}

void Codegen::emit_throw(const AstNode *node)
{
    SourceNote here(*this, node);
    bool bare = node->a == NULL;
    if (bare && in_handler == 0) {
        error("bare throw is not in a handler");
        return;
    }
    note_unwind();
    if (bare && !catch_open.empty())
        catch_open.back() = 0;

    std::vector<llvm::Value *> args;
    llvm::Function *target = bare ? rethrow_fn() : throw_fn();
    if (!bare) {
        RValue value = emit_rvalue(node->a);
        if (failed || value.v == NULL || value.ty.ir == NULL)
            return;
        uint64_t bytes = module.getDataLayout().getTypeAllocSize(value.ty.ir);
        if (bytes == 0)
            bytes = 1;
        llvm::Value *size = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), bytes);
        llvm::Value *slot = builder.CreateCall(
            allocate_exception()->getFunctionType(), allocate_exception(), {size});
        builder.CreateStore(value.v, slot);
        auto *null_dtor = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
        args.push_back(slot);
        args.push_back(type_info(value.ty));
        args.push_back(null_dtor);
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
    SourceNote here(*this, node);
    struct Caught {
        llvm::BasicBlock *body;
        llvm::Constant *info;
        bool catch_all;
        Ty ty;
        const AstNode *clause;
    };

    note_unwind();
    llvm::BasicBlock *pad = new_block("lpad");
    llvm::BasicBlock *next = new_block("after");
    handlers.push_back(pad);
    emit_compound(node->a);
    if (!failed && !terminated())
        builder.CreateBr(next);
    handlers.pop_back();
    if (failed)
        return;

    std::vector<Caught> caught;
    builder.SetInsertPoint(pad);
    auto *pad_ty = llvm::StructType::get(llvm::PointerType::get(context, 0),
                                          llvm::Type::getInt32Ty(context));
    for (const AstNode *clause = node->b; clause != NULL; clause = clause->next) {
        Caught one;
        one.body = new_block("catch");
        one.clause = clause;
        one.catch_all = clause->text != NULL && std::strcmp(clause->text, "...") == 0;
        one.info = NULL;
        if (!one.catch_all) {
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
            one.ty = ty;
            one.info = type_info(ty);
        }
        caught.push_back(one);
    }

    llvm::LandingPadInst *landing = builder.CreateLandingPad(pad_ty, caught.size(), "pad");
    for (const Caught &one : caught) {
        if (one.catch_all)
            landing->addClause(llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0)));
        else
            landing->addClause(one.info);
    }
    if (caught.empty()) {
        builder.CreateResume(landing);
        builder.SetInsertPoint(next);
        return;
    }

    llvm::Value *exn = builder.CreateExtractValue(landing, 0, "exn");
    bool needs_resume = caught.size() > 1;
    for (const Caught &one : caught) {
        if (one.catch_all)
            needs_resume = false;
    }
    llvm::BasicBlock *resume = needs_resume ? new_block("resume") : NULL;
    if (caught.size() == 1) {
        builder.CreateBr(caught[0].body);
    } else {
        llvm::Value *selector = builder.CreateExtractValue(landing, 1, "selector");
        llvm::Function *types = typeid_fn();
        for (unsigned i = 0; i < caught.size(); i++) {
            bool last = i + 1 == caught.size();
            if (caught[i].catch_all) {
                builder.CreateBr(caught[i].body);
                break;
            }
            llvm::Value *id = builder.CreateCall(types->getFunctionType(), types, {caught[i].info}, "typeid");
            llvm::Value *match = builder.CreateICmpEQ(selector, id, "match");
            llvm::BasicBlock *cont = last ? resume : new_block("next.catch");
            builder.CreateCondBr(match, caught[i].body, cont);
            builder.SetInsertPoint(cont);
        }
        if (resume != NULL && builder.GetInsertBlock() == resume)
            builder.CreateResume(landing);
    }

    llvm::Function *begin = begin_catch_fn();
    llvm::Function *end = end_catch_fn();
    for (unsigned i = 0; i < caught.size() && !failed; i++) {
        const Caught &one = caught[i];
        builder.SetInsertPoint(one.body);
        llvm::Value *object = builder.CreateCall(begin->getFunctionType(), begin, {exn}, "caught");
        catch_open.push_back(1);
        in_handler++;
        push_scope();
        if (!one.catch_all && one.clause->a != NULL && one.clause->a->kind == AST_PARAMETER
            && one.clause->a->b != NULL) {
            Storage storage;
            Ty ty = parse_specs(one.clause->a->a, Where::Local, &storage);
            Declared declared = declare_one(ty, one.clause->a->b);
            if (!failed && declared.name != NULL && declared.name->text != NULL) {
                llvm::AllocaInst *addr = alloca_entry(declared.ty.ir, declared.name->text);
                bind(declared.name->text, Slot{addr, declared.ty});
                if (object != NULL && declared.ty.ir != NULL) {
                    llvm::Value *loaded = builder.CreateLoad(declared.ty.ir, object, declared.name->text);
                    builder.CreateStore(loaded, addr);
                }
            }
        }
        if (!failed)
            emit_compound(one.clause->b);
        pop_scope();
        in_handler--;
        if (!failed && !terminated()) {
            if (!catch_open.empty() && catch_open.back()) {
                builder.CreateCall(end->getFunctionType(), end, {});
                catch_open.back() = 0;
            }
            builder.CreateBr(next);
        }
        if (!catch_open.empty())
            catch_open.pop_back();
    }
    if (!failed)
        builder.SetInsertPoint(next);
}
