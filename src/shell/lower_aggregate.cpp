/*
 * Structs, unions, enums, arrays, and initializers.
 */
#include "lower.hpp"

Ty Codegen::record_type(const Record &rec)
{
    Ty ty;
    ty.ir = rec.ir;
    ty.is_record = true;
    ty.is_union = rec.is_union;
    ty.record = rec.name;
    return ty;
}

Ty Codegen::enum_type(const std::string &name)
{
    Ty ty = int_ty();
    ty.is_enum = true;
    ty.record = name;
    return ty;
}

unsigned Codegen::record_align(const Ty &ty)
{
    unsigned align = 1;
    auto found = records.find(ty.record);
    if (found == records.end())
        return align;
    for (const Field &field : found->second.fields) {
        llvm::Type *storage = field.bits > 0 ? field.storage : field.ty.ir;
        unsigned one = abi_align(storage);
        if (one > align)
            align = one;
    }
    return align;
}

std::string Codegen::tag_key(bool is_union, const char *text)
{
    std::string base = text == NULL || text[0] == '\0'
        ? (is_union ? ".union.anon." : ".struct.anon.") + std::to_string(++anon_records)
        : (is_union ? "union." : "struct.") + std::string(text);
    if (ns_stack.empty())
        return base;
    return current_path() + "::" + base;
}

Ty Codegen::make_record(const AstNode *node)
{
    SourceNote here(focus, node);
    bool is_union = node->kind == AST_UNION;
    std::string key = tag_key(is_union, node->text);
    if (node->a == NULL) {
        auto found = records.find(key);
        if (found == records.end()) {
            Record created;
            created.name = key;
            created.is_union = is_union;
            created.ir = llvm::StructType::create(context, key);
            records.emplace(key, created);
        } else if (found->second.is_union != is_union) {
            error("tag is the wrong kind");
            return int_ty();
        }
        return record_type(records[key]);
    }
    auto found = records.find(key);
    if (found != records.end() && (found->second.complete || found->second.defining)) {
        error("redefined type");
        return found->second.ir == NULL ? int_ty() : record_type(found->second);
    }
    if (found != records.end() && found->second.is_union != is_union) {
        error("tag is the wrong kind");
        return int_ty();
    }
    Record created;
    created.name = key;
    created.is_union = is_union;
    created.defining = true;
    created.ir = found == records.end() ? llvm::StructType::create(context, key)
                                        : found->second.ir;
    records[key] = created;
    for (const AstNode *mem = node->a; mem != NULL && !failed; mem = mem->next) {
        if (mem->kind != AST_DECLARATION) {
            error("not a member");
            break;
        }
        Storage storage;
        Ty base = parse_specs(mem->a, Where::Param, &storage);
        if (failed)
            break;
        if (storage.is_typedef || storage.is_extern || storage.is_static || storage.is_inline
            || storage.is_register) {
            error("storage class is not allowed here");
            break;
        }
        if (mem->b == NULL) {
            error("member has no name");
            break;
        }
        for (const AstNode *item = mem->b; item != NULL && !failed; item = item->next) {
            Declared declared = declare_one(base, item);
            if (failed)
                break;
            if (declared.func != NULL) {
                error("not a member");
                break;
            }
            if (declared.name == NULL || declared.name->text == NULL
                || declared.name->kind != AST_IDENT) {
                if (declared.bits > 0)
                    error("unnamed bit-field");
                else if (declared.name != NULL && declared.name->kind == AST_QUALIFIED)
                    error("qualified names are a later phase");
                else
                    error("member has no name");
                break;
            }
            if (declared.bits > 0
                && (declared.ty.is_pointer || declared.ty.is_array || declared.ty.is_record
                    || declared.ty.is_float || !is_integer(declared.ty))) {
                error("bit-field is not an integer");
                break;
            }
            if (declared.bits == 0 && !complete_type(declared.ty)) {
                error("incomplete member");
                break;
            }
            Field field;
            field.name = declared.name->text;
            field.ty = declared.ty;
            field.bits = declared.bits;
            field.storage = declared.bits > 0
                ? llvm::Type::getIntNTy(context, static_cast<unsigned>(declared.bits))
                : declared.ty.ir;
            for (const Field &earlier : records[key].fields) {
                if (earlier.name == field.name)
                    error_name("name is already defined", field.name.c_str());
            }
            if (failed)
                break;
            records[key].fields.push_back(field);
        }
    }
    if (failed)
        return int_ty();
    Record &rec = records[key];
    if (rec.fields.empty()) {
        error("empty struct");
        return int_ty();
    }
    std::vector<llvm::Type *> elems;
    if (rec.is_union) {
        uint64_t size = 1;
        for (Field &field : rec.fields) {
            field.index = 0;
            llvm::Type *storage = field.bits > 0 ? field.storage : field.ty.ir;
            uint64_t bytes = module.getDataLayout().getTypeAllocSize(storage).getFixedValue();
            if (bytes > size)
                size = bytes;
        }
        elems.push_back(llvm::ArrayType::get(llvm::Type::getInt8Ty(context), size));
    } else {
        for (Field &field : rec.fields) {
            field.index = static_cast<unsigned>(elems.size());
            elems.push_back(field.bits > 0 ? field.storage : field.ty.ir);
        }
    }
    if (rec.ir->isOpaque())
        rec.ir->setBody(elems);
    rec.complete = true;
    rec.defining = false;
    return record_type(rec);
}

Ty Codegen::make_enum(const AstNode *node)
{
    SourceNote here(focus, node);
    std::string key = node->text != NULL && node->text[0] != '\0'
        ? qualify(std::string("enum.") + node->text)
        : qualify(".enum.anon." + std::to_string(++anon_records));
    if (node->a == NULL) {
        if (!enum_types.count(key)) {
            error("incomplete enum");
            return int_ty();
        }
        return enum_type(key);
    }
    if (enum_types.count(key)) {
        error("redefined type");
        return enum_type(key);
    }
    int64_t next = 0;
    for (const AstNode *item = node->a; item != NULL && !failed; item = item->next) {
        if (item->kind != AST_ENUMERATOR || item->text == NULL) {
            error("not an enumerator");
            break;
        }
        int64_t value = next;
        if (item->a != NULL && !const_i64(item->a, value))
            break;
        std::string enumerator = qualify(item->text);
        if (enum_consts.count(enumerator) != 0) {
            error_name("name is already defined", item->text);
            break;
        }
        enum_consts[enumerator] = value;
        next = value + 1;
    }
    if (failed)
        return int_ty();
    enum_types.insert(key);
    return enum_type(key);
}

std::optional<Field> Codegen::find_field(const Ty &object, const char *name)
{
    if (!object.is_record || name == NULL) {
        error("not a struct");
        return std::nullopt;
    }
    auto found = records.find(object.record);
    if (found == records.end() || !found->second.complete) {
        error("incomplete type");
        return std::nullopt;
    }
    for (const Field &field : found->second.fields) {
        if (field.name == name)
            return field;
    }
    error_name("no such member", name);
    return std::nullopt;
}

Slot Codegen::field_addr(llvm::Value *base, const Ty &object, const Field &field)
{
    Slot slot;
    slot.ty = field.ty;
    slot.bits = field.bits;
    if (object.is_union)
        slot.addr = base;
    else {
        auto found = records.find(object.record);
        llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
        llvm::Value *index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), field.index);
        slot.addr = builder.CreateInBoundsGEP(found->second.ir, base, {zero, index});
    }
    return slot;
}

RValue Codegen::pointer_binary(const char *op, RValue left, RValue right, bool compare)
{
    bool add = std::strcmp(op, "+") == 0;
    bool sub = std::strcmp(op, "-") == 0;
    if (compare) {
        if (!left.ty.is_pointer || !right.ty.is_pointer || !same_value_type(left.ty, right.ty)) {
            error("invalid operands");
            return fallback();
        }
        llvm::CmpInst::Predicate pred = llvm::CmpInst::ICMP_EQ;
        if (std::strcmp(op, "<") == 0)
            pred = llvm::CmpInst::ICMP_ULT;
        else if (std::strcmp(op, ">") == 0)
            pred = llvm::CmpInst::ICMP_UGT;
        else if (std::strcmp(op, "<=") == 0)
            pred = llvm::CmpInst::ICMP_ULE;
        else if (std::strcmp(op, ">=") == 0)
            pred = llvm::CmpInst::ICMP_UGE;
        else if (std::strcmp(op, "!=") == 0)
            pred = llvm::CmpInst::ICMP_NE;
        RValue out;
        out.ty = bool_ty();
        out.v = builder.CreateICmp(pred, left.v, right.v);
        return out;
    }
    Ty index_ty = integer(64, true, 4);
    if ((add || sub) && left.ty.is_pointer && left.ty.inner != NULL && is_integer(right.ty)) {
        llvm::Value *index = convert(right.v, right.ty, index_ty);
        if (sub)
            index = builder.CreateNeg(index);
        RValue out;
        out.ty = left.ty;
        out.v = builder.CreateInBoundsGEP(left.ty.inner->ir, left.v, {index});
        return out;
    }
    if (add && right.ty.is_pointer && right.ty.inner != NULL && is_integer(left.ty)) {
        llvm::Value *index = convert(left.v, left.ty, index_ty);
        RValue out;
        out.ty = right.ty;
        out.v = builder.CreateInBoundsGEP(right.ty.inner->ir, right.v, {index});
        return out;
    }
    if (sub && left.ty.is_pointer && right.ty.is_pointer && same_value_type(left.ty, right.ty)
        && left.ty.inner != NULL) {
        RValue out;
        out.ty = index_ty;
        out.v = builder.CreatePtrDiff(left.ty.inner->ir, left.v, right.v);
        return out;
    }
    error("invalid operands");
    return fallback();
}

bool Codegen::plan_record(const Record &rec, const AstNode *list, std::vector<const AstNode *> &out)
{
    out.assign(rec.fields.size(), nullptr);
    int cursor = 0;
    for (const AstNode *item = list; item != NULL && !failed; item = item->next) {
        if (item->kind == AST_DESIGNATED) {
            if (item->b == NULL) {
                error("nested designators are not in this phase");
                return false;
            }
            if (item->text == NULL) {
                error("array designator in a struct");
                return false;
            }
            int index = -1;
            for (std::size_t i = 0; i < rec.fields.size(); i++) {
                if (rec.fields[i].name == item->text)
                    index = static_cast<int>(i);
            }
            if (index < 0) {
                error_name("no such member", item->text);
                return false;
            }
            out[static_cast<std::size_t>(index)] = item->b;
            cursor = index + 1;
            continue;
        }
        if (cursor >= static_cast<int>(rec.fields.size())) {
            error("too many initializers");
            return false;
        }
        out[static_cast<std::size_t>(cursor++)] = item;
    }
    return !failed;
}

bool Codegen::plan_array(const Ty &ty, const AstNode *list, std::vector<const AstNode *> &out)
{
    out.assign(static_cast<std::size_t>(ty.array_len), nullptr);
    int cursor = 0;
    for (const AstNode *item = list; item != NULL && !failed; item = item->next) {
        if (item->kind == AST_DESIGNATED) {
            if (item->b == NULL) {
                error("nested designators are not in this phase");
                return false;
            }
            if (item->text != NULL) {
                error("struct designator in an array");
                return false;
            }
            int64_t index = 0;
            if (!const_i64(item->a, index) || index < 0 || index >= ty.array_len) {
                if (!failed)
                    error("array designator is out of range");
                return false;
            }
            out[static_cast<std::size_t>(index)] = item->b;
            cursor = static_cast<int>(index) + 1;
            continue;
        }
        if (cursor >= ty.array_len) {
            error("too many initializers");
            return false;
        }
        out[static_cast<std::size_t>(cursor++)] = item;
    }
    return !failed;
}

bool Codegen::union_choice(const Record &rec, const AstNode *list, int &index, const AstNode *&value)
{
    value = NULL;
    index = 0;
    if (list == NULL)
        return true;
    if (list->kind == AST_DESIGNATED) {
        if (list->next != NULL) {
            error("too many initializers");
            return false;
        }
        if (list->b == NULL) {
            error("nested designators are not in this phase");
            return false;
        }
        if (list->text == NULL) {
            error("array designator in a struct");
            return false;
        }
        for (std::size_t i = 0; i < rec.fields.size(); i++) {
            if (rec.fields[i].name == list->text) {
                index = static_cast<int>(i);
                value = list->b;
                return true;
            }
        }
        error_name("no such member", list->text);
        return false;
    }
    if (list->next != NULL) {
        error("too many initializers");
        return false;
    }
    value = list;
    return true;
}

llvm::Constant *Codegen::field_constant(const Field &field, const AstNode *init)
{
    llvm::Type *storage = field.bits > 0 ? field.storage : field.ty.ir;
    if (init == NULL)
        return llvm::Constant::getNullValue(storage);
    if (init->kind == AST_INIT) {
        if (field.bits > 0) {
            error("bit-field initializer is not scalar");
            return llvm::Constant::getNullValue(storage);
        }
        return const_initializer(field.ty, init);
    }
    Const value = eval_const(init);
    if (!value.ok) {
        if (!failed)
            error("file-scope initializer is not constant");
        return llvm::Constant::getNullValue(storage);
    }
    llvm::Constant *wide = llvm::cast<llvm::Constant>(convert_const(value.v, value.ty, field.ty));
    if (field.bits == 0)
        return wide;
    Ty narrow = field.ty;
    narrow.ir = storage;
    narrow.is_bool = false;
    return llvm::cast<llvm::Constant>(convert_const(wide, field.ty, narrow));
}

llvm::Constant *Codegen::const_initializer(const Ty &ty, const AstNode *init)
{
    if (failed)
        return llvm::Constant::getNullValue(ty.ir != NULL ? ty.ir : llvm::Type::getInt32Ty(context));
    if (init != NULL && init->kind != AST_INIT) {
        if (ty.is_array || (ty.is_record && init->kind != AST_INIT)) {
            if (!(ty.is_record && !ty.is_array)) {
                error("cannot initialize an aggregate from a value");
                return llvm::Constant::getNullValue(ty.ir);
            }
        }
        Const value = eval_const(init);
        if (!value.ok) {
            if (!failed)
                error("file-scope initializer is not constant");
            return llvm::Constant::getNullValue(ty.ir);
        }
        return llvm::cast<llvm::Constant>(convert_const(value.v, value.ty, ty));
    }
    const AstNode *list = init == NULL ? NULL : init->a;
    if (ty.is_record && !ty.is_union) {
        auto found = records.find(ty.record);
        if (found != records.end() && found->second.is_class) {
            error("brace initialization of a class is not in this phase");
            return llvm::Constant::getNullValue(ty.ir);
        }
        std::vector<const AstNode *> planned;
        if (found == records.end() || !plan_record(found->second, list, planned))
            return llvm::Constant::getNullValue(ty.ir);
        std::vector<llvm::Constant *> elems;
        for (std::size_t i = 0; i < found->second.fields.size(); i++)
            elems.push_back(field_constant(found->second.fields[i], planned[i]));
        return llvm::ConstantStruct::get(found->second.ir, elems);
    }
    if (ty.is_array && ty.inner != NULL) {
        std::vector<const AstNode *> planned;
        if (!plan_array(ty, list, planned))
            return llvm::Constant::getNullValue(ty.ir);
        std::vector<llvm::Constant *> elems;
        for (const AstNode *item : planned) {
            if (item == NULL)
                elems.push_back(llvm::Constant::getNullValue(ty.inner->ir));
            else
                elems.push_back(const_initializer(*ty.inner, item));
        }
        return llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(ty.ir), elems);
    }
    if (ty.is_union) {
        auto found = records.find(ty.record);
        const AstNode *value = NULL;
        int index = 0;
        if (found == records.end() || !union_choice(found->second, list, index, value))
            return llvm::Constant::getNullValue(ty.ir);
        if (value == NULL)
            return llvm::Constant::getNullValue(ty.ir);
        llvm::Constant *member = field_constant(
            found->second.fields[static_cast<std::size_t>(index)], value);
        auto *bytes_ty = llvm::cast<llvm::ArrayType>(found->second.ir->getElementType(0));
        uint64_t size = bytes_ty->getNumElements();
        unsigned width = static_cast<unsigned>(size * 8);
        llvm::APInt bits(width, 0);
        if (auto *integer = llvm::dyn_cast<llvm::ConstantInt>(member))
            bits = integer->getValue().zextOrTrunc(width);
        else if (auto *fp = llvm::dyn_cast<llvm::ConstantFP>(member))
            bits = fp->getValueAPF().bitcastToAPInt().zextOrTrunc(width);
        else {
            error("union initializer is not a scalar");
            return llvm::Constant::getNullValue(ty.ir);
        }
        std::vector<uint8_t> bytes;
        for (uint64_t i = 0; i < size; i++)
            bytes.push_back(static_cast<uint8_t>(
                bits.extractBits(8, static_cast<unsigned>(i * 8)).getZExtValue()));
        llvm::Constant *raw = llvm::ConstantDataArray::get(context, llvm::ArrayRef<uint8_t>(bytes));
        return llvm::ConstantStruct::get(found->second.ir, {raw});
    }
    if (list == NULL)
        return llvm::Constant::getNullValue(ty.ir);
    if (list->kind == AST_DESIGNATED || list->next != NULL) {
        error("too many initializers");
        return llvm::Constant::getNullValue(ty.ir);
    }
    return const_initializer(ty, list);
}

void Codegen::store_initializer(const Slot &slot, const AstNode *init)
{
    SourceNote here(focus, init);
    if (failed)
        return;
    if (init != NULL && init->kind != AST_INIT) {
        if (slot.ty.is_array) {
            error("cannot initialize an array from a value");
            return;
        }
        RValue value = cast_to(emit_rvalue(init), slot.ty);
        if (!failed)
            store_slot(slot, value.v, true);
        return;
    }
    const AstNode *list = init == NULL ? NULL : init->a;
    if (slot.ty.is_record && !slot.ty.is_union) {
        auto found = records.find(slot.ty.record);
        if (found != records.end() && found->second.is_class) {
            error("brace initialization of a class is not in this phase");
            return;
        }
        std::vector<const AstNode *> planned;
        if (found == records.end() || !plan_record(found->second, list, planned))
            return;
        for (std::size_t i = 0; i < found->second.fields.size(); i++) {
            Slot field = field_addr(slot.addr, slot.ty, found->second.fields[i]);
            if (planned[i] == NULL)
                store_slot(field, llvm::Constant::getNullValue(field.ty.ir), true);
            else
                store_initializer(field, planned[i]);
        }
        return;
    }
    if (slot.ty.is_array && slot.ty.inner != NULL) {
        std::vector<const AstNode *> planned;
        if (!plan_array(slot.ty, list, planned))
            return;
        for (std::size_t i = 0; i < planned.size(); i++) {
            llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
            llvm::Value *index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), i);
            Slot elem;
            elem.ty = *slot.ty.inner;
            elem.addr = builder.CreateInBoundsGEP(slot.ty.ir, slot.addr, {zero, index});
            if (planned[i] == NULL)
                store_slot(elem, llvm::Constant::getNullValue(elem.ty.ir), true);
            else
                store_initializer(elem, planned[i]);
        }
        return;
    }
    if (slot.ty.is_union) {
        auto found = records.find(slot.ty.record);
        const AstNode *value = NULL;
        int index = 0;
        if (found == records.end() || !union_choice(found->second, list, index, value))
            return;
        if (value == NULL) {
            store_slot(slot, llvm::Constant::getNullValue(slot.ty.ir), true);
            return;
        }
        Slot field = field_addr(slot.addr, slot.ty, found->second.fields[static_cast<std::size_t>(index)]);
        store_initializer(field, value);
        return;
    }
    if (list == NULL) {
        store_slot(slot, llvm::Constant::getNullValue(slot.ty.ir), true);
        return;
    }
    if (list->kind == AST_DESIGNATED || list->next != NULL) {
        error("too many initializers");
        return;
    }
    store_initializer(slot, list);
}
