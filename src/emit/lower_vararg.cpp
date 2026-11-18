/*
 * Variadic reads. <stdarg> calls these builtins.
 * The list is the record llvm.va_start fills on
 * this host: gp_offset, fp_offset, the overflow
 * area, and the register save area. An integer or
 * a pointer uses a general register. A float uses
 * an SSE register. Later arguments come from the
 * stack. va_copy copies that record, so the two
 * lists then walk independently. va_end is a
 * no-op here and is safe on a copy.
 */
#include "lower.hpp"

const char *Codegen::vararg_builtin(const AstNode *callee) const
{
    if (callee == NULL || callee->kind != AST_IDENT || callee->text == NULL)
        return NULL;
    if (std::strcmp(callee->text, "__builtin_va_start") == 0)
        return callee->text;
    if (std::strcmp(callee->text, "__builtin_va_arg") == 0)
        return callee->text;
    if (std::strcmp(callee->text, "__builtin_va_end") == 0)
        return callee->text;
    if (std::strcmp(callee->text, "__builtin_va_copy") == 0)
        return callee->text;
    return NULL;
}

bool Codegen::va_list_record(const Ty &ty)
{
    if (!ty.is_record || ty.ir == NULL || !ty.ir->isStructTy())
        return false;
    auto found = records.find(ty.record);
    if (found == records.end() || found->second.fields.size() != 4)
        return false;
    const std::vector<Field> &fields = found->second.fields;
    if (!is_integer(fields[0].ty))
        return false;
    if (fields[0].ty.ir->getIntegerBitWidth() != 32)
        return false;
    if (!is_integer(fields[1].ty))
        return false;
    if (fields[1].ty.ir->getIntegerBitWidth() != 32)
        return false;
    if (!fields[2].ty.is_pointer || !fields[3].ty.is_pointer)
        return false;
    auto *layout_ty = llvm::cast<llvm::StructType>(ty.ir);
    const llvm::DataLayout &data = module.getDataLayout();
    const llvm::StructLayout *layout = data.getStructLayout(layout_ty);
    uint64_t bytes = data.getTypeAllocSize(ty.ir).getFixedValue();
    if (bytes != 24)
        return false;
    if (layout->getElementOffset(0) != 0 || layout->getElementOffset(1) != 4)
        return false;
    if (layout->getElementOffset(2) != 8 || layout->getElementOffset(3) != 16)
        return false;
    return true;
}

bool Codegen::is_va_list(const Ty &ty)
{
    return ty.is_pointer && ty.inner != NULL && va_list_record(*ty.inner);
}

bool Codegen::va_slot(const Ty &ty)
{
    if (ty.ir == NULL || ty.is_void || ty.is_array || ty.is_record)
        return false;
    if (ty.is_function)
        return false;
    if (!ty.is_pointer && !ty.is_float && !is_integer(ty))
        return false;
    unsigned align = abi_align(ty.ir);
    return align > 0 && align <= 8;
}

llvm::Function *Codegen::va_intrinsic(llvm::Intrinsic::ID id)
{
    llvm::Type *ptr = llvm::PointerType::get(context, 0);
    return llvm::Intrinsic::getOrInsertDeclaration(&module, id, {ptr});
}

llvm::Value *Codegen::va_next(
        llvm::Value *list,
        llvm::Type *layout,
        const Ty &value)
{
    bool fp = value.is_float && !value.is_pointer;
    unsigned field = fp ? 1u : 0u;
    unsigned limit = fp ? 160u : 40u;
    unsigned step = fp ? 16u : 8u;
    const llvm::DataLayout &data = module.getDataLayout();
    uint64_t bytes = data.getTypeAllocSize(value.ir).getFixedValue();
    unsigned stack = 8;
    if (bytes > 8)
        stack = static_cast<unsigned>((bytes + 7u) & ~7u);
    llvm::Type *i32 = llvm::Type::getInt32Ty(context);
    llvm::Type *i8 = llvm::Type::getInt8Ty(context);
    llvm::Type *ptr = llvm::PointerType::get(context, 0);
    llvm::Value *off_ptr = builder.CreateStructGEP(layout, list, field);
    llvm::Value *offset = builder.CreateLoad(i32, off_ptr);
    llvm::Value *in_reg = builder.CreateICmpULE(
            offset, llvm::ConstantInt::get(i32, limit));
    llvm::BasicBlock *reg_bb = new_block("va.reg");
    llvm::BasicBlock *mem_bb = new_block("va.mem");
    llvm::BasicBlock *end_bb = new_block("va.end");
    builder.CreateCondBr(in_reg, reg_bb, mem_bb);

    builder.SetInsertPoint(reg_bb);
    llvm::Value *save_ptr = builder.CreateStructGEP(layout, list, 3);
    llvm::Value *save = builder.CreateLoad(ptr, save_ptr);
    llvm::Value *reg_addr = builder.CreateGEP(i8, save, offset);
    llvm::Value *next = builder.CreateAdd(
            offset, llvm::ConstantInt::get(i32, step));
    builder.CreateStore(next, off_ptr);
    builder.CreateBr(end_bb);

    builder.SetInsertPoint(mem_bb);
    llvm::Value *over_ptr = builder.CreateStructGEP(layout, list, 2);
    llvm::Value *over = builder.CreateLoad(ptr, over_ptr);
    llvm::Value *advanced = builder.CreateGEP(
            i8, over, llvm::ConstantInt::get(i32, stack));
    builder.CreateStore(advanced, over_ptr);
    builder.CreateBr(end_bb);

    builder.SetInsertPoint(end_bb);
    llvm::PHINode *phi = builder.CreatePHI(ptr, 2);
    phi->addIncoming(reg_addr, reg_bb);
    phi->addIncoming(over, mem_bb);
    return phi;
}

static unsigned arg_count(const AstNode *list)
{
    unsigned count = 0;
    for (const AstNode *arg = list; arg != NULL; arg = arg->next)
        count++;
    return count;
}

Ty Codegen::type_vararg(const AstNode *node)
{
    const char *kind = vararg_builtin(node->a);
    unsigned count = arg_count(node->b);
    unsigned need = 1;
    if (kind != NULL && (std::strcmp(kind, "__builtin_va_arg") == 0
            || std::strcmp(kind, "__builtin_va_copy") == 0))
        need = 2;
    if (count != need) {
        error("argument count");
        return int_ty();
    }
    const AstNode *first = node->b;
    const AstNode *second = first != NULL ? first->next : NULL;
    Ty list = type_of(first);
    if (failed)
        return int_ty();
    if (!is_va_list(list)) {
        error("not a va_list");
        return int_ty();
    }
    if (kind != NULL && std::strcmp(kind, "__builtin_va_copy") == 0) {
        Ty src = type_of(second);
        if (failed)
            return int_ty();
        if (!is_va_list(src)) {
            error("not a va_list");
            return int_ty();
        }
    }
    if (kind != NULL && std::strcmp(kind, "__builtin_va_arg") == 0) {
        Ty probe = type_of(second);
        if (failed)
            return int_ty();
        Ty value = probe.inner != NULL ? drop(*probe.inner) : int_ty();
        if (!probe.is_pointer || probe.inner == NULL || !va_slot(value)) {
            error("va_arg type is not supported");
            return int_ty();
        }
        return pointer_to(void_ty());
    }
    return void_ty();
}

RValue Codegen::emit_vararg(const AstNode *node)
{
    const char *kind = vararg_builtin(node->a);
    unsigned count = arg_count(node->b);
    unsigned need = 1;
    bool arg = kind != NULL && std::strcmp(kind, "__builtin_va_arg") == 0;
    bool copy = kind != NULL && std::strcmp(kind, "__builtin_va_copy") == 0;
    bool start = kind != NULL && std::strcmp(kind, "__builtin_va_start") == 0;
    if (arg || copy)
        need = 2;
    if (count != need) {
        error("argument count");
        return fallback();
    }
    if (function == NULL || (start && !function->isVarArg())) {
        error("va_start is not in a variadic function");
        return fallback();
    }
    const Ty *saved = expect;
    expect = nullptr;
    RValue list = emit_rvalue(node->b);
    expect = saved;
    if (failed)
        return fallback();
    if (!is_va_list(list.ty) || list.ty.inner == NULL) {
        error("not a va_list");
        return fallback();
    }
    if (list.ty.inner->ir == NULL) {
        error("not a va_list");
        return fallback();
    }
    if (copy) {
        expect = nullptr;
        RValue src = emit_rvalue(node->b->next);
        expect = saved;
        if (failed)
            return fallback();
        if (!is_va_list(src.ty)) {
            error("not a va_list");
            return fallback();
        }
        llvm::Function *intr = va_intrinsic(llvm::Intrinsic::vacopy);
        builder.CreateCall(intr, {list.v, src.v});
        RValue out;
        out.ty = void_ty();
        return out;
    }
    if (arg) {
        expect = nullptr;
        RValue probe = emit_rvalue(node->b->next);
        expect = saved;
        if (failed)
            return fallback();
        if (!probe.ty.is_pointer || probe.ty.inner == NULL) {
            error("va_arg type is not supported");
            return fallback();
        }
        Ty value = drop(*probe.ty.inner);
        if (!va_slot(value)) {
            error("va_arg type is not supported");
            return fallback();
        }
        llvm::Value *addr = va_next(list.v, list.ty.inner->ir, value);
        RValue out;
        out.ty = pointer_to(void_ty());
        out.v = addr;
        return out;
    }
    llvm::Intrinsic::ID id = llvm::Intrinsic::vaend;
    if (start)
        id = llvm::Intrinsic::vastart;
    llvm::Function *intr = va_intrinsic(id);
    builder.CreateCall(intr, {list.v});
    RValue out;
    out.ty = void_ty();
    return out;
}
