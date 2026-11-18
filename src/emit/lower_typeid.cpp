/*
 * typeid(expression) and typeid(type).
 *
 * The result is a pointer to one TypeInfo for that type, so == and !=
 * are pointer comparisons. A polymorphic glvalue loads the TypeInfo
 * from the slot before the vtable's first function. That slot is the
 * most derived type. Other operands are not evaluated.
 */
#include "lower.hpp"

#include "llvm/IR/Comdat.h"

static bool type_glvalue(const AstNode *node)
{
    if (node == NULL)
        return false;
    if (node->kind == AST_IDENT || node->kind == AST_QUALIFIED
        || node->kind == AST_MEMBER || node->kind == AST_ARROW
        || node->kind == AST_INDEX)
        return true;
    return node->kind == AST_UNARY && node->text != NULL
        && std::strcmp(node->text, "*") == 0;
}

static const char *integer_name(unsigned bits, bool is_signed, bool readable)
{
    if (bits == 8)
        return is_signed ? "char" : (readable ? "unsigned char" : "unsigned-char");
    if (bits == 16)
        return is_signed ? "short" : (readable ? "unsigned short" : "unsigned-short");
    if (bits == 32)
        return is_signed ? "int" : "unsigned";
    if (bits == 64)
        return is_signed ? "long" : (readable ? "unsigned long" : "unsigned-long");
    return NULL;
}

llvm::StructType *Codegen::typeinfo_layout()
{
    auto found = records.find("class.TypeInfo");
    if (found != records.end() && found->second.complete && found->second.ir != NULL
        && found->second.ir->isStructTy()) {
        auto *layout = llvm::cast<llvm::StructType>(found->second.ir);
        if (layout->getNumElements() > 0)
            return layout;
    }
    if (info_layout == NULL) {
        auto *ptr = llvm::PointerType::get(context, 0);
        info_layout = llvm::StructType::create(context, {ptr}, "typeinfo.layout");
    }
    return info_layout;
}

Ty Codegen::typeid_type()
{
    auto found = records.find("class.TypeInfo");
    if (found != records.end() && found->second.complete && found->second.ir != NULL) {
        Ty inner = record_type(found->second);
        inner.is_const = true;
        return pointer_to(inner);
    }
    Ty inner;
    inner.ir = typeinfo_layout();
    inner.is_record = true;
    inner.record = "typeinfo";
    inner.is_const = true;
    return pointer_to(inner);
}

std::string Codegen::describe_type(const Ty &ty, bool top, bool readable) const
{
    std::string text;
    if (!top && ty.is_const)
        text += readable ? "const " : "c.";
    if (!top && ty.is_volatile)
        text += readable ? "volatile " : "v.";
    if (ty.is_void)
        return text + "void";
    if (ty.is_bool)
        return text + "bool";
    if (ty.is_pointer && ty.inner != NULL) {
        text += describe_type(*ty.inner, false, readable);
        text += readable ? " *" : "*";
        return text;
    }
    if (ty.is_array && ty.inner != NULL) {
        text += describe_type(*ty.inner, false, readable);
        text += "[";
        text += std::to_string(ty.array_len);
        text += "]";
        return text;
    }
    if (ty.is_record) {
        if (readable) {
            auto found = records.find(ty.record);
            if (found != records.end())
                return text + source_name(found->second);
        }
        return text + ty.record;
    }
    if (ty.is_enum)
        return text + ty.record;
    if (ty.is_float)
        return text + (ty.fp_rank >= 2 ? "double" : "float");
    if (ty.ir != NULL && ty.ir->isIntegerTy()) {
        unsigned bits = ty.ir->getIntegerBitWidth();
        const char *name = integer_name(bits, ty.is_signed, readable);
        if (name != NULL)
            return text + name;
        return text + "i" + std::to_string(bits);
    }
    return text + "unknown";
}

llvm::Constant *Codegen::type_descriptor(const Ty &ty)
{
    std::string key = describe_type(ty, true, false);
    std::string symbol = "typeinfo." + key;
    llvm::GlobalVariable *global = module.getGlobalVariable(symbol, true);
    if (global != NULL)
        return global;

    llvm::StructType *layout = typeinfo_layout();
    if (layout == NULL || layout->getNumElements() == 0
        || !layout->getElementType(0)->isPointerTy()) {
        error("TypeInfo has no name");
        return NULL;
    }
    std::string label = describe_type(ty, true, true);
    llvm::Constant *bytes = llvm::ConstantDataArray::getString(context, label, true);
    llvm::Comdat *group = module.getOrInsertComdat(symbol);
    auto *name_g = new llvm::GlobalVariable(
        module, bytes->getType(), true, llvm::GlobalValue::LinkOnceODRLinkage,
        bytes, "typename." + key);
    name_g->setComdat(group);
    std::vector<llvm::Constant *> fields;
    fields.push_back(name_g);
    for (unsigned i = 1; i < layout->getNumElements(); i++)
        fields.push_back(llvm::Constant::getNullValue(layout->getElementType(i)));
    llvm::Constant *init = llvm::ConstantStruct::get(layout, fields);
    global = new llvm::GlobalVariable(
        module, layout, true, llvm::GlobalValue::LinkOnceODRLinkage, init, symbol);
    global->setComdat(group);
    return global;
}

llvm::Value *Codegen::dynamic_type(llvm::Value *object, const std::string &key)
{
    if (object == NULL) {
        error("typeid has no object");
        return NULL;
    }
    auto *ptr_ty = llvm::PointerType::get(context, 0);
    llvm::Value *table = builder.CreateLoad(ptr_ty, vptr_addr(object, key));
    llvm::Value *minus = llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(context), -1);
    llvm::Value *slot = builder.CreateGEP(ptr_ty, table, {minus}, "ti.slot");
    return builder.CreateLoad(ptr_ty, slot, "dynamic.type");
}

RValue Codegen::emit_typeid(const AstNode *node)
{
    SourceNote here(*this, node);
    const AstNode *operand = node == NULL ? NULL : node->a;
    if (operand == NULL) {
        error("typeid has no operand");
        return fallback();
    }
    bool type_operand = is_type_form(operand);
    Ty static_ty = type_operand ? type_from_name(operand) : type_of(operand);
    if (failed)
        return fallback();
    static_ty = drop(static_ty);
    Ty result = typeid_type();
    bool dynamic = !type_operand && static_ty.is_record && polymorphic(static_ty.record)
        && type_glvalue(operand);
    if (dynamic) {
        std::optional<Slot> slot = lvalue(operand);
        if (!slot || slot->addr == NULL)
            return fallback();
        llvm::Value *info = dynamic_type(slot->addr, static_ty.record);
        if (info == NULL)
            return fallback();
        return RValue{info, result};
    }
    llvm::Constant *desc = type_descriptor(static_ty);
    if (desc == NULL)
        return fallback();
    return RValue{desc, result};
}
