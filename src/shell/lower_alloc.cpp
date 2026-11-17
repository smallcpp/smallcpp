/*
 * new and the allocation safepoint.
 */
#include "lower.hpp"

Ty Codegen::null_ty()
{
    Ty ty = pointer_to(int_ty());
    ty.is_null = true;
    return ty;
}

llvm::Function *Codegen::allocator()
{
    llvm::Function *fn = module.getFunction("smallcpp_alloc");
    if (fn != NULL)
        return fn;
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *i64 = llvm::Type::getInt64Ty(context);
    auto *type = llvm::FunctionType::get(ptr, {i64, i64}, false);
    return llvm::Function::Create(type, llvm::GlobalValue::ExternalLinkage, "smallcpp_alloc", module);
}

Ty Codegen::new_element(const AstNode *node, std::vector<const AstNode *> &bounds)
{
    if (node == NULL) {
        error("new has no type");
        return int_ty();
    }
    if (node->kind == AST_ARRAY) {
        bounds.push_back(node->b);
        return new_element(node->a, bounds);
    }
    if (node->kind == AST_POINTER)
        return pointer_to(new_element(node->a != NULL ? node->a : node->b, bounds));
    if (node->kind == AST_SPECIFIER || node->kind == AST_STRUCT || node->kind == AST_UNION
        || node->kind == AST_ENUM || node->kind == AST_CLASS || node->kind == AST_TYPE_NAME)
        return parse_specs(node, Where::Param);
    if (node->kind == AST_IDENT) {
        std::optional<Ty> alias = lookup_alias(node->text);
        if (!alias) {
            error_name("unknown type", node->text);
            return int_ty();
        }
        return *alias;
    }
    error("new has no type");
    return int_ty();
}

std::vector<llvm::Value *> Codegen::live_pointers()
{
    std::vector<llvm::Value *> live;
    if (function == NULL)
        return live;
    for (llvm::Argument &arg : function->args()) {
        if (arg.getType()->isPointerTy())
            live.push_back(&arg);
    }
    for (std::size_t i = 1; i < scopes.size(); i++) {
        for (const auto &item : scopes[i]) {
            if (item.second.ty.is_pointer && item.second.addr != NULL)
                live.push_back(item.second.addr);
        }
    }
    return live;
}

llvm::Value *Codegen::allocate(llvm::Value *bytes, llvm::Value *count)
{
    if (function != NULL && !function->hasGC())
        function->setGC("statepoint-example");
    std::vector<llvm::Value *> live = live_pointers();
    llvm::CallInst *state = builder.CreateGCStatepointCall(
        0, 0, allocator(), {bytes, count}, std::nullopt, live, "safepoint");
    auto *ptr = llvm::PointerType::get(context, 0);
    llvm::Value *result = builder.CreateGCResult(state, ptr, "allocated");
    std::vector<llvm::Value *> mapped = live;
    mapped.push_back(result);
    llvm::Function *stackmap = llvm::Intrinsic::getOrInsertDeclaration(
        &module, llvm::Intrinsic::experimental_stackmap);
    std::vector<llvm::Value *> args;
    args.push_back(llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0));
    args.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0));
    args.insert(args.end(), mapped.begin(), mapped.end());
    builder.CreateCall(stackmap, args);
    return result;
}

RValue Codegen::emit_new(const AstNode *node)
{
    std::vector<const AstNode *> bounds;
    const AstNode *allocated = node->a;
    if (allocated != NULL && allocated->kind == AST_POINTER)
        allocated = allocated->a;
    Ty elem = new_element(allocated, bounds);
    if (failed)
        return fallback();
    if (elem.is_void || !complete_type(elem.is_array ? *elem.inner : elem)) {
        error("incomplete type");
        return fallback();
    }
    Ty object = elem.is_array ? *elem.inner : elem;
    if (class_type(object) && records[object.record].abstract) {
        error("abstract class");
        return fallback();
    }
    auto *i64 = llvm::Type::getInt64Ty(context);
    uint64_t bytes = module.getDataLayout().getTypeAllocSize(object.ir).getFixedValue();
    if (class_type(object) && records[object.record].complete_size != 0)
        bytes = records[object.record].complete_size;
    llvm::Value *count = llvm::ConstantInt::get(i64, 1);
    for (const AstNode *bound : bounds) {
        RValue value = emit_rvalue(bound);
        if (failed)
            return fallback();
        if (!is_integer(value.ty)) {
            error("array bound is not an integer");
            return fallback();
        }
        llvm::Value *wide = convert(value.v, value.ty, integer(64, true, 4));
        count = builder.CreateMul(count, wide);
    }
    llvm::Value *result = allocate(llvm::ConstantInt::get(i64, bytes), count);
    if (class_type(object)) {
        std::string plain = source_name(records[object.record]);
        auto pos = plain.rfind("::");
        std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
        std::string ir_name = plain + "::" + last;
        if (overloads.count(ir_name) != 0) {
            std::vector<RValue> args;
            for (const AstNode *arg = node->b; arg != NULL && !failed; arg = arg->next)
                args.push_back(emit_rvalue(arg));
            if (failed)
                return fallback();
            std::vector<Ty> types;
            types.push_back(pointer_to(object));
            for (const RValue &arg : args)
                types.push_back(arg.ty);
            Func *ctor = resolve_keys({ir_name}, types, last.c_str());
            if (ctor == NULL)
                return fallback();
            std::vector<llvm::Value *> values;
            values.push_back(result);
            unsigned user = static_cast<unsigned>(ctor->params.size() - 1);
            unsigned provided = static_cast<unsigned>(args.size());
            unsigned limit = provided < user ? provided : user;
            for (unsigned i = 0; i < limit; i++)
                values.push_back(convert(args[i].v, args[i].ty, ctor->params[i + 1]));
            for (unsigned i = limit; i < user; i++) {
                if (i + 1 < ctor->names.size() && ctor->names[i + 1] == ".most")
                    values.push_back(ctor_flag(true));
                else
                    values.push_back(ctor->defaults[i + 1]);
            }
            builder.CreateCall(ctor->fn->getFunctionType(), ctor->fn, values);
        } else if (node->b != NULL) {
            error("no matching constructor");
            return fallback();
        } else if (!records[object.record].vbases.empty()) {
            emit_virtual_bases(result, object.record, NULL);
        }
    } else if (node->b != NULL) {
        error("the type has no constructor");
        return fallback();
    }
    if (!failed && class_type(object))
        emit_vptrs(result, object.record);
    if (!failed && bounds.empty())
        bind_destructor(result, object);
    RValue out;
    out.ty = pointer_to(object);
    out.v = result;
    return out;
}

llvm::Function *Codegen::set_dtor_fn()
{
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {ptr, ptr}, false);
    return declare_runtime("smallcpp_set_dtor", type);
}

llvm::Function *Codegen::dtor_done_fn()
{
    auto *ptr = llvm::PointerType::get(context, 0);
    auto *type = llvm::FunctionType::get(llvm::Type::getVoidTy(context), {ptr}, false);
    return declare_runtime("smallcpp_dtor_done", type);
}

Func *Codegen::destructor_of(const Ty &ty)
{
    if (!class_type(ty))
        return NULL;
    auto found = records.find(ty.record);
    if (found == records.end())
        return NULL;
    std::string plain = source_name(found->second);
    auto pos = plain.rfind("::");
    std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
    std::string ir_name = plain + "::~" + last;
    auto over = overloads.find(ir_name);
    if (over == overloads.end())
        return NULL;
    for (Func &fn : over->second) {
        if (fn.is_dtor && fn.fn != NULL)
            return &fn;
    }
    return NULL;
}

void Codegen::bind_destructor(llvm::Value *object, const Ty &ty)
{
    Func *dtor = destructor_of(ty);
    if (object == NULL || dtor == NULL || dtor->fn == NULL)
        return;
    llvm::Function *set = set_dtor_fn();
    builder.CreateCall(set->getFunctionType(), set, {object, dtor->fn});
}

void Codegen::emit_delete(const AstNode *node)
{
    RValue ptr = emit_rvalue(node->a);
    if (failed)
        return;
    if (!ptr.ty.is_pointer || ptr.ty.inner == NULL) {
        error("delete operand is not a pointer");
        return;
    }
    Func *dtor = destructor_of(*ptr.ty.inner);
    if (dtor == NULL || dtor->fn == NULL || ptr.v == NULL)
        return;
    auto *null = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
    llvm::BasicBlock *call_bb = new_block("delete.dtor");
    llvm::BasicBlock *done_bb = new_block("delete.done");
    llvm::Value *skip = builder.CreateICmpEQ(ptr.v, null, "delete.null");
    builder.CreateCondBr(skip, done_bb, call_bb);
    builder.SetInsertPoint(call_bb);
    llvm::Value *callee = dtor->fn;
    if (dtor->vindex >= 0 && ptr.ty.inner->is_record) {
        auto *ptr_ty = llvm::PointerType::get(context, 0);
        llvm::Value *table = builder.CreateLoad(ptr_ty, vptr_addr(ptr.v, ptr.ty.inner->record));
        llvm::Value *index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                                    static_cast<uint64_t>(dtor->vindex));
        llvm::Value *slot = builder.CreateInBoundsGEP(ptr_ty, table, {index});
        callee = builder.CreateLoad(ptr_ty, slot);
    }
    builder.CreateCall(dtor->fn->getFunctionType(), callee, {ptr.v});
    builder.CreateBr(done_bb);
    builder.SetInsertPoint(done_bb);
}
