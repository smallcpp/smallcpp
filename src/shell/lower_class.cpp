/*
 * Classes, methods, bases, and virtual calls.
 */
#include "lower.hpp"

#include "llvm/IR/DataLayout.h"

std::string Codegen::class_key_of(const char *plain) const
{
    if (plain == NULL)
        return "";
    if (ns_stack.empty())
        return std::string("class.") + plain;
    return current_path() + "::class." + plain;
}

std::string Codegen::source_name(const Record &rec) const
{
    auto pos = rec.name.rfind("class.");
    if (pos == std::string::npos)
        return rec.name;
    return rec.name.substr(0, pos) + rec.name.substr(pos + 6);
}

std::string Codegen::base_field_name(const std::string &key) const
{
    return ".base." + key;
}

bool Codegen::class_type(const Ty &ty) const
{
    if (!ty.is_record)
        return false;
    auto found = records.find(ty.record);
    return found != records.end() && found->second.is_class;
}

bool Codegen::derives_from(const std::string &derived, const std::string &base) const
{
    if (derived == base)
        return true;
    auto found = records.find(derived);
    if (found == records.end())
        return false;
    for (const std::string &one : found->second.bases) {
        if (derives_from(one, base))
            return true;
    }
    for (const std::string &one : found->second.direct_virtual) {
        if (derives_from(one, base))
            return true;
    }
    return false;
}

bool Codegen::can_access(const std::string &access, const std::string &owner) const
{
    if (access.empty() || access == "public")
        return true;
    if (method_class.empty())
        return false;
    if (method_class == owner)
        return true;
    return access == "protected" && derives_from(method_class, owner);
}

Record *Codegen::class_named(const char *plain)
{
    if (plain == NULL)
        return NULL;
    std::optional<Ty> alias = lookup_alias(plain);
    if (alias && alias->is_record) {
        auto found = records.find(alias->record);
        if (found != records.end() && found->second.is_class)
            return &found->second;
    }
    auto found = records.find(class_key_of(plain));
    if (found != records.end() && found->second.is_class)
        return &found->second;
    return NULL;
}

bool Codegen::quals_const(const AstNode *list, bool &is_const)
{
    is_const = false;
    for (const AstNode *node = list; node != NULL; node = node->next) {
        if (node->kind != AST_SPECIFIER || node->text == NULL) {
            error("not a qualifier");
            return false;
        }
        if (std::strcmp(node->text, "const") == 0)
            is_const = true;
        else if (std::strcmp(node->text, "volatile") != 0) {
            error("not a qualifier");
            return false;
        }
    }
    return true;
}

const char *Codegen::node_name(const AstNode *node) const
{
    if (node == NULL)
        return NULL;
    return node->text;
}

void Codegen::install_method(const std::string &key, MethodDecl decl, Ty ret, std::vector<Param> params,
                    const AstNode *body, const AstNode *inits)
{
    if (failed)
        return;
    Ty self = pointer_to(record_type(records[key]));
    if (decl.is_const && self.inner != NULL)
        self.inner->is_const = true;
    Param self_param;
    self_param.name = "this";
    self_param.ty = self;
    params.insert(params.begin(), std::move(self_param));
    Func incoming;
    incoming.source = decl.ir_name;
    incoming.home = ns_stack;
    incoming.lookup = live_chain();
    incoming.ret = ret;
    incoming.body = body;
    incoming.klass = key;
    incoming.method_const = decl.is_const;
    incoming.is_ctor = decl.is_ctor;
    incoming.is_dtor = decl.is_dtor;
    incoming.is_pure = decl.is_pure;
    incoming.inits = inits;
    if (decl.is_ctor && !records[key].vbases.empty()) {
        Param most;
        most.name = ".most";
        most.ty = int_ty();
        most.fallback = llvm::ConstantInt::get(most.ty.ir, 1);
        params.push_back(std::move(most));
    }
    for (const Param &param : params) {
        incoming.params.push_back(param.ty);
        incoming.defaults.push_back(param.fallback);
        incoming.names.push_back(param.name);
    }
    install(std::move(incoming));
    if (failed)
        return;
    for (const MethodDecl &existing : records[key].methods) {
        if (existing.ir_name == decl.ir_name && existing.is_const == decl.is_const
            && existing.is_ctor == decl.is_ctor && existing.is_dtor == decl.is_dtor)
            return;
    }
    records[key].methods.push_back(decl);
}

void Codegen::layout_class(Record &rec)
{
    if (rec.fields.empty()) {
        Field pad;
        pad.name = ".pad";
        pad.ty = integer(8, true, 1);
        pad.access = "private";
        rec.fields.push_back(pad);
    }
    std::vector<llvm::Type *> elems;
    for (Field &field : rec.fields) {
        field.index = static_cast<unsigned>(elems.size());
        elems.push_back(field.ty.ir);
    }
    if (rec.ir->isOpaque())
        rec.ir->setBody(elems);
    rec.complete = true;
    rec.defining = false;
}

bool Codegen::polymorphic(const std::string &key) const
{
    auto found = records.find(key);
    if (found == records.end())
        return false;
    if (found->second.vptr >= 0 || found->second.vtable != NULL || !found->second.vslots.empty())
        return true;
    for (const std::string &base : found->second.bases) {
        if (polymorphic(base))
            return true;
    }
    return false;
}

bool Codegen::vptr_at_start(const std::string &key) const
{
    auto found = records.find(key);
    if (found == records.end())
        return false;
    if (found->second.vptr == 0)
        return true;
    if (found->second.primary_base.empty() || found->second.bases.empty())
        return false;
    if (found->second.bases[0] != found->second.primary_base)
        return false;
    return vptr_at_start(found->second.primary_base);
}

std::string Codegen::virtual_key(const Ty &ret, const std::vector<Param> &params, bool is_const)
{
    std::string text = is_const ? "c:" : "m:";
    text += type_tag(ret);
    for (const Param &param : params)
        text += "," + type_tag(param.ty);
    return text;
}

std::string Codegen::virtual_key(const Func &fn)
{
    std::string text = fn.method_const ? "c:" : "m:";
    text += type_tag(fn.ret);
    for (std::size_t i = 1; i < fn.params.size(); i++)
        text += "," + type_tag(fn.params[i]);
    return text;
}

bool Codegen::hides_virtual(const std::string &base, const std::string &name, const std::string &vkey) const
{
    auto found = records.find(base);
    if (found == records.end())
        return false;
    for (const VSlot &slot : found->second.vslots) {
        if (slot.name == name && slot.vkey == vkey)
            return true;
    }
    for (const std::string &one : found->second.bases) {
        if (hides_virtual(one, name, vkey))
            return true;
    }
    for (const std::string &one : found->second.direct_virtual) {
        if (hides_virtual(one, name, vkey))
            return true;
    }
    return false;
}

Func *Codegen::declared_virtual(const std::string &klass, const std::string &name, const std::string &vkey)
{
    auto found = records.find(klass);
    if (found == records.end())
        return NULL;
    for (const MethodDecl &method : found->second.methods) {
        if (!method.is_virtual || method.is_ctor)
            continue;
        if (vkey == "dtor") {
            if (!method.is_dtor)
                continue;
        } else if (method.is_dtor || method.name != name) {
            continue;
        }
        auto group = overloads.find(method.ir_name);
        if (group == overloads.end())
            continue;
        for (Func &fn : group->second) {
            if (fn.klass != klass || fn.fn == NULL || fn.method_const != method.is_const)
                continue;
            if (vkey == "dtor" || virtual_key(fn) == vkey)
                return &fn;
        }
    }
    return NULL;
}

Func *Codegen::overrider_along(const std::string &klass, const std::vector<std::string> &path, std::size_t step,
                               const std::string &name, const std::string &vkey)
{
    Func *here = declared_virtual(klass, name, vkey);
    if (here != NULL)
        return here;
    if (step >= path.size())
        return NULL;
    return overrider_along(path[step], path, step + 1, name, vkey);
}

uint64_t Codegen::path_offset(const std::string &derived, const std::vector<std::string> &path) const
{
    uint64_t offset = 0;
    std::string at = derived;
    for (const std::string &step : path) {
        auto found = records.find(at);
        if (found == records.end() || found->second.ir == NULL)
            return offset;
        const llvm::StructLayout *layout = module.getDataLayout().getStructLayout(found->second.ir);
        const Field *field = NULL;
        for (const Field &one : found->second.fields) {
            if (one.name == base_field_name(step))
                field = &one;
        }
        if (field == NULL)
            return offset;
        offset += layout->getElementOffset(field->index);
        at = step;
    }
    return offset;
}

uint64_t Codegen::thunk_delta(const std::string &complete, const std::vector<std::string> &path,
                              const std::string &impl) const
{
    uint64_t sub = path_offset(complete, path);
    if (impl.empty() || impl == complete)
        return sub;
    std::vector<std::string> prefix;
    for (const std::string &step : path) {
        prefix.push_back(step);
        if (step == impl)
            return sub - path_offset(complete, prefix);
    }
    return sub;
}

llvm::GlobalVariable *Codegen::emit_vtable(const std::string &name, const std::vector<llvm::Constant *> &slots)
{
    if (slots.empty())
        return NULL;
    auto *elem = llvm::PointerType::get(context, 0);
    auto *array = llvm::ArrayType::get(elem, slots.size());
    return new llvm::GlobalVariable(
        module, array, true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(array, slots), name);
}

llvm::Function *Codegen::make_thunk(llvm::Function *impl, uint64_t delta, const std::string &name)
{
    if (impl == NULL || delta == 0)
        return impl;
    llvm::FunctionType *type = impl->getFunctionType();
    auto *thunk = llvm::Function::Create(type, llvm::GlobalValue::InternalLinkage, name, &module);
    auto *block = llvm::BasicBlock::Create(context, "entry", thunk);
    llvm::IRBuilder<> ir(block);
    llvm::Value *self = thunk->getArg(0);
    llvm::Value *adjusted = ir.CreateGEP(
        llvm::Type::getInt8Ty(context), self,
        llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(context), -static_cast<int64_t>(delta)));
    std::vector<llvm::Value *> args;
    args.push_back(adjusted);
    for (unsigned i = 1; i < thunk->arg_size(); i++)
        args.push_back(thunk->getArg(i));
    llvm::CallInst *call = ir.CreateCall(impl, args);
    if (type->getReturnType()->isVoidTy())
        ir.CreateRetVoid();
    else
        ir.CreateRet(call);
    return thunk;
}

llvm::Value *Codegen::base_ptr(llvm::Value *ptr, const std::string &from, const std::string &base)
{
    auto found = records.find(from);
    if (found == records.end())
        return ptr;
    Ty type = record_type(found->second);
    for (const Field &field : found->second.fields) {
        if (field.name == base_field_name(base))
            return field_addr(ptr, type, field).addr;
    }
    return ptr;
}

llvm::Value *Codegen::vptr_addr(llvm::Value *ptr, const std::string &key)
{
    auto found = records.find(key);
    if (found == records.end())
        return ptr;
    Record &rec = found->second;
    if (rec.vptr >= 0) {
        Ty type = record_type(rec);
        return field_addr(ptr, type, rec.fields[static_cast<std::size_t>(rec.vptr)]).addr;
    }
    if (!rec.primary_base.empty())
        return vptr_addr(base_ptr(ptr, key, rec.primary_base), rec.primary_base);
    return ptr;
}

llvm::GlobalVariable *Codegen::view_for(const std::string &complete, const std::vector<std::string> &path)
{
    auto found = records.find(complete);
    if (found == records.end())
        return NULL;
    for (const VTable &table : found->second.vtables) {
        if (table.path == path)
            return table.global;
    }
    return NULL;
}

void Codegen::lay_bases(llvm::Value *ptr, const std::string &complete, const std::string &sub,
                        std::vector<std::string> &path)
{
    auto found = records.find(sub);
    if (found == records.end())
        return;
    for (const std::string &base : found->second.bases) {
        if (!polymorphic(base))
            continue;
        path.push_back(base);
        llvm::Value *child = base_ptr(ptr, sub, base);
        if (base != found->second.primary_base) {
            llvm::GlobalVariable *table = view_for(complete, path);
            if (table != NULL)
                builder.CreateStore(table, vptr_addr(child, base));
        }
        lay_bases(child, complete, base, path);
        path.pop_back();
    }
}

VTable Codegen::make_view(const std::string &complete, const std::string &base,
                          const std::vector<std::string> &path)
{
    VTable table;
    table.base = base;
    table.path = path;
    auto layout = records.find(base);
    if (layout == records.end())
        return table;
    auto *null_fn = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
    std::vector<llvm::Constant *> slots;
    int index = 0;
    for (const VSlot &slot : layout->second.vslots) {
        Func *over = overrider_along(complete, path, 0, slot.name, slot.vkey);
        llvm::Function *fn = over != NULL ? over->fn : slot.fn;
        std::string impl = over != NULL ? over->klass : slot.impl_class;
        uint64_t delta = fn == NULL ? 0 : thunk_delta(complete, path, impl);
        if (fn != NULL && delta != 0) {
            fn = make_thunk(fn, delta,
                            records[complete].name + ".thunk."
                                + std::to_string(records[complete].vtables.size()) + "."
                                + std::to_string(index));
        }
        slots.push_back(fn != NULL ? static_cast<llvm::Constant *>(fn) : null_fn);
        index++;
    }
    table.global = emit_vtable(
        records[complete].name + ".vt." + std::to_string(records[complete].vtables.size()), slots);
    return table;
}

void Codegen::build_views(const std::string &complete, const std::string &sub, std::vector<std::string> &path)
{
    auto found = records.find(sub);
    if (found == records.end())
        return;
    std::vector<std::string> bases = found->second.bases;
    std::string primary = found->second.primary_base;
    for (const std::string &base : bases) {
        if (!polymorphic(base))
            continue;
        path.push_back(base);
        if (base != primary)
            records[complete].vtables.push_back(make_view(complete, base, path));
        build_views(complete, base, path);
        path.pop_back();
    }
}

void Codegen::emit_vptrs(llvm::Value *ptr, const std::string &key)
{
    auto found = records.find(key);
    if (found == records.end() || ptr == NULL)
        return;
    if (found->second.vtable == NULL && found->second.vtables.empty())
        return;
    if (found->second.vtable != NULL)
        builder.CreateStore(found->second.vtable, vptr_addr(ptr, key));
    std::vector<std::string> path;
    lay_bases(ptr, key, key, path);
}

void Codegen::build_vtable(const std::string &key)
{
    Record &rec = records[key];
    if (!rec.bases.empty() && vptr_at_start(rec.bases[0]))
        rec.primary_base = rec.bases[0];
    if (!rec.primary_base.empty())
        rec.vslots = records[rec.primary_base].vslots;
    for (MethodDecl &method : rec.methods) {
        if (!method.is_virtual || method.is_ctor)
            continue;
        auto group = overloads.find(method.ir_name);
        if (group == overloads.end())
            continue;
        for (Func &fn : group->second) {
            if (fn.klass != key || fn.fn == NULL || fn.method_const != method.is_const)
                continue;
            if (method.is_dtor != fn.is_dtor)
                continue;
            std::string vkey = method.is_dtor ? "dtor" : virtual_key(fn);
            bool replaced = false;
            for (VSlot &slot : rec.vslots) {
                bool same = method.is_dtor ? slot.vkey == "dtor"
                                           : slot.name == method.name && slot.vkey == vkey;
                if (!same)
                    continue;
                slot.fn = fn.fn;
                slot.impl_class = key;
                slot.pure = method.is_pure;
                method.vindex = slot.index;
                fn.vindex = slot.index;
                replaced = true;
                break;
            }
            if (replaced)
                continue;
            VSlot slot;
            slot.name = method.name;
            slot.vkey = vkey;
            slot.impl_class = key;
            slot.pure = method.is_pure;
            slot.index = static_cast<int>(rec.vslots.size());
            slot.fn = fn.fn;
            method.vindex = slot.index;
            fn.vindex = slot.index;
            rec.vslots.push_back(slot);
        }
    }
    auto *null_fn = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
    std::vector<llvm::Constant *> slots;
    for (const VSlot &slot : rec.vslots)
        slots.push_back(slot.fn != NULL ? static_cast<llvm::Constant *>(slot.fn) : null_fn);
    rec.vtable = emit_vtable(rec.name + ".vt", slots);
    std::vector<std::string> path;
    build_views(key, key, path);
    rec.abstract = false;
    for (const VSlot &slot : rec.vslots) {
        if (slot.pure)
            rec.abstract = true;
    }
    std::vector<std::string> pending_bases = rec.bases;
    pending_bases.insert(pending_bases.end(), rec.direct_virtual.begin(), rec.direct_virtual.end());
    for (const std::string &base : pending_bases) {
        std::vector<std::string> walk;
        std::set<std::string> seen;
        walk.push_back(base);
        while (!walk.empty()) {
            std::string one = walk.back();
            walk.pop_back();
            if (!seen.insert(one).second)
                continue;
            auto found = records.find(one);
            if (found == records.end())
                continue;
            for (const VSlot &slot : found->second.vslots) {
                if (slot.pure && !overrides_slot(key, slot))
                    rec.abstract = true;
            }
            walk.insert(walk.end(), found->second.bases.begin(), found->second.bases.end());
            walk.insert(walk.end(), found->second.direct_virtual.begin(), found->second.direct_virtual.end());
        }
    }
}

void Codegen::collect_vbases(const std::string &key, std::vector<std::string> &out,
                             std::set<std::string> &seen) const
{
    auto found = records.find(key);
    if (found == records.end())
        return;
    for (const std::string &base : found->second.direct_virtual) {
        if (!seen.insert(base).second)
            continue;
        collect_vbases(base, out, seen);
        out.push_back(base);
    }
    for (const std::string &base : found->second.bases)
        collect_vbases(base, out, seen);
}

bool Codegen::field_path(const std::string &complete, const std::vector<std::string> &path) const
{
    std::string at = complete;
    for (const std::string &step : path) {
        auto found = records.find(at);
        if (found == records.end())
            return false;
        bool saw = false;
        for (const std::string &base : found->second.bases) {
            if (base == step)
                saw = true;
        }
        if (!saw)
            return false;
        at = step;
    }
    return true;
}

uint64_t Codegen::vbase_at(const std::string &complete, const std::string &base) const
{
    auto found = records.find(complete);
    if (found == records.end())
        return 0;
    for (std::size_t i = 0; i < found->second.vbases.size(); i++) {
        if (found->second.vbases[i] == base && i < found->second.vbase_off.size())
            return found->second.vbase_off[i];
    }
    return 0;
}

uint64_t Codegen::subobject_at(const std::string &complete, const std::vector<std::string> &path) const
{
    if (path.empty())
        return 0;
    if (field_path(complete, path))
        return path_offset(complete, path);
    if (path.size() == 1)
        return vbase_at(complete, path[0]);
    return 0;
}

void Codegen::add_vbtable(const std::string &complete, const std::string &sub,
                          const std::vector<std::string> &path)
{
    auto layout = records.find(sub);
    auto owner = records.find(complete);
    if (layout == records.end() || owner == records.end() || layout->second.vbases.empty())
        return;
    int64_t base_at = static_cast<int64_t>(subobject_at(complete, path));
    std::vector<llvm::Constant *> slots;
    auto *i64 = llvm::Type::getInt64Ty(context);
    for (const std::string &base : layout->second.vbases) {
        int64_t delta = static_cast<int64_t>(vbase_at(complete, base)) - base_at;
        slots.push_back(llvm::ConstantInt::getSigned(i64, delta));
    }
    auto *array = llvm::ArrayType::get(i64, slots.size());
    std::string name = owner->second.name + ".vb";
    for (const std::string &step : path)
        name += "." + step;
    auto *global = new llvm::GlobalVariable(
        module, array, true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(array, slots), name);
    VBTable table;
    table.path = path;
    table.global = global;
    owner->second.vbtables.push_back(table);
}

void Codegen::build_vbtables(const std::string &complete, const std::string &sub,
                             std::vector<std::string> &path)
{
    auto found = records.find(sub);
    if (found == records.end())
        return;
    if (found->second.vbptr >= 0)
        add_vbtable(complete, sub, path);
    std::vector<std::string> bases = found->second.bases;
    std::vector<std::string> virtuals = found->second.direct_virtual;
    for (const std::string &base : bases) {
        path.push_back(base);
        build_vbtables(complete, base, path);
        path.pop_back();
    }
    for (const std::string &base : virtuals) {
        if (records[base].vbptr < 0)
            continue;
        path.push_back(base);
        add_vbtable(complete, base, path);
        path.pop_back();
    }
}

void Codegen::place_vbases(const std::string &key)
{
    Record &rec = records[key];
    if (rec.ir == NULL)
        return;
    const llvm::DataLayout &layout = module.getDataLayout();
    if (rec.vbases.empty()) {
        rec.complete_size = layout.getTypeAllocSize(rec.ir).getFixedValue();
        return;
    }
    uint64_t place = layout.getTypeAllocSize(rec.ir).getFixedValue();
    uint64_t class_align = layout.getABITypeAlign(rec.ir).value();
    if (class_align == 0)
        class_align = 1;
    rec.vbase_off.clear();
    for (const std::string &base : rec.vbases) {
        llvm::StructType *ty = records[base].ir;
        uint64_t align = ty == NULL ? 1 : layout.getABITypeAlign(ty).value();
        if (align == 0)
            align = 1;
        place = (place + align - 1) & ~(align - 1);
        rec.vbase_off.push_back(place);
        uint64_t size = ty == NULL ? 0 : layout.getTypeAllocSize(ty).getFixedValue();
        place += size;
    }
    place = (place + class_align - 1) & ~(class_align - 1);
    rec.complete_size = place;
    std::vector<std::string> path;
    build_vbtables(key, key, path);
}

llvm::GlobalVariable *Codegen::vbtable_for(const std::string &complete,
                                           const std::vector<std::string> &path)
{
    auto found = records.find(complete);
    if (found == records.end())
        return NULL;
    for (const VBTable &table : found->second.vbtables) {
        if (table.path == path)
            return table.global;
    }
    return NULL;
}

void Codegen::store_vbptrs(llvm::Value *complete_ptr, llvm::Value *sub_ptr, const std::string &complete,
                           const std::string &sub, std::vector<std::string> &path)
{
    auto found = records.find(sub);
    if (found == records.end())
        return;
    if (found->second.vbptr >= 0) {
        llvm::GlobalVariable *table = vbtable_for(complete, path);
        if (table != NULL) {
            Ty type = record_type(found->second);
            llvm::Value *slot = field_addr(
                sub_ptr, type, found->second.fields[static_cast<std::size_t>(found->second.vbptr)]).addr;
            builder.CreateStore(table, slot);
        }
    }
    std::vector<std::string> bases = found->second.bases;
    std::vector<std::string> virtuals = found->second.direct_virtual;
    for (const std::string &base : bases) {
        path.push_back(base);
        store_vbptrs(complete_ptr, base_ptr(sub_ptr, sub, base), complete, base, path);
        path.pop_back();
    }
    for (const std::string &base : virtuals) {
        if (records[base].vbptr < 0)
            continue;
        llvm::Value *child = builder.CreateGEP(
            llvm::Type::getInt8Ty(context), complete_ptr,
            llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(context),
                                         static_cast<int64_t>(vbase_at(complete, base))));
        path.push_back(base);
        store_vbptrs(complete_ptr, child, complete, base, path);
        path.pop_back();
    }
}

void Codegen::emit_vbptrs(llvm::Value *self, const std::string &key)
{
    std::vector<std::string> path;
    store_vbptrs(self, self, key, key, path);
}

bool Codegen::has_constructor(const std::string &key)
{
    auto found = records.find(key);
    if (found == records.end())
        return false;
    std::string plain = source_name(found->second);
    auto pos = plain.rfind("::");
    std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
    return overloads.count(plain + "::" + last) != 0;
}

bool Codegen::virtual_destructor(const std::string &key) const
{
    auto found = records.find(key);
    if (found == records.end())
        return false;
    for (const MethodDecl &method : found->second.methods) {
        if (method.is_dtor && method.is_virtual)
            return true;
    }
    for (const VSlot &slot : found->second.vslots) {
        if (slot.vkey == "dtor")
            return true;
    }
    for (const std::string &base : found->second.bases) {
        if (virtual_destructor(base))
            return true;
    }
    for (const std::string &base : found->second.direct_virtual) {
        if (virtual_destructor(base))
            return true;
    }
    return false;
}

bool Codegen::overrides_slot(const std::string &key, const VSlot &slot)
{
    if (slot.vkey == "dtor") {
        auto found = records.find(key);
        if (found == records.end())
            return false;
        for (const MethodDecl &method : found->second.methods) {
            if (method.is_dtor && method.is_virtual)
                return true;
        }
        return false;
    }
    return declared_virtual(key, slot.name, slot.vkey) != NULL;
}

llvm::Value *Codegen::vbase_addr(llvm::Value *ptr, const std::string &from, const std::string &base)
{
    auto found = records.find(from);
    if (found == records.end() || found->second.vbptr < 0)
        return ptr;
    int index = -1;
    for (std::size_t i = 0; i < found->second.vbases.size(); i++) {
        if (found->second.vbases[i] == base)
            index = static_cast<int>(i);
    }
    if (index < 0)
        return ptr;
    Ty type = record_type(found->second);
    llvm::Value *slot = field_addr(
        ptr, type, found->second.fields[static_cast<std::size_t>(found->second.vbptr)]).addr;
    auto *i64 = llvm::Type::getInt64Ty(context);
    auto *ptr_ty = llvm::PointerType::get(context, 0);
    llvm::Value *table = builder.CreateLoad(ptr_ty, slot);
    llvm::Value *entry = builder.CreateInBoundsGEP(
        i64, table, {llvm::ConstantInt::get(i64, static_cast<uint64_t>(index))});
    llvm::Value *offset = builder.CreateLoad(i64, entry);
    return builder.CreateGEP(llvm::Type::getInt8Ty(context), ptr, offset);
}

llvm::Value *Codegen::ctor_flag(bool most)
{
    return llvm::ConstantInt::get(int_ty().ir, most ? 1 : 0);
}

void Codegen::emit_pure_stub(Func &fn)
{
    if (fn.fn == NULL)
        return;
    auto *block = llvm::BasicBlock::Create(context, "entry", fn.fn);
    builder.SetInsertPoint(block);
    builder.CreateUnreachable();
}

void Codegen::emit_virtual_bases(llvm::Value *self, const std::string &key, const AstNode *inits)
{
    auto found = records.find(key);
    if (found == records.end() || found->second.vbases.empty())
        return;
    std::vector<std::string> bases = found->second.vbases;
    std::vector<uint64_t> offsets = found->second.vbase_off;
    emit_vbptrs(self, key);
    for (std::size_t i = 0; i < bases.size() && i < offsets.size(); i++) {
        const AstNode *args = NULL;
        std::string plain = source_name(records[bases[i]]);
        auto pos = plain.rfind("::");
        std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
        for (const AstNode *init = inits; init != NULL; init = init->next) {
            if (init->kind == AST_MEM_INIT && init->text != NULL && last == init->text)
                args = init->a;
        }
        if (args == NULL && !has_constructor(bases[i]))
            continue;
        llvm::Value *addr = builder.CreateGEP(
            llvm::Type::getInt8Ty(context), self,
            llvm::ConstantInt::getSigned(llvm::Type::getInt64Ty(context),
                                         static_cast<int64_t>(offsets[i])));
        emit_ctor_call(addr, bases[i], args, records[bases[i]].vbases.empty());
    }
}

Ty Codegen::make_class(const AstNode *node)
{
    const char *plain = node_name(node->a);
    if (plain == NULL || plain[0] == '\0') {
        error("anonymous class");
        return int_ty();
    }
    std::string key = class_key_of(plain);
    bool definition = node->b != NULL || node->c != NULL;
    auto found = records.find(key);
    if (!definition) {
        if (found == records.end()) {
            Record created;
            created.name = key;
            created.is_class = true;
            created.ir = llvm::StructType::create(context, key);
            records.emplace(key, created);
            record_alias(qualify(plain), record_type(records[key]));
        } else if (!found->second.is_class) {
            error("tag is the wrong kind");
            return int_ty();
        }
        return record_type(records[key]);
    }
    if (found != records.end() && (found->second.complete || found->second.defining)) {
        error("redefined type");
        return found->second.ir == NULL ? int_ty() : record_type(found->second);
    }
    if (found != records.end() && !found->second.is_class) {
        error("tag is the wrong kind");
        return int_ty();
    }
    Record created;
    created.name = key;
    created.is_class = true;
    created.defining = true;
    created.ir = found == records.end() ? llvm::StructType::create(context, key) : found->second.ir;
    records[key] = created;
    record_alias(qualify(plain), record_type(records[key]));
    if (failed)
        return int_ty();
    std::string source = source_name(records[key]);
    for (const AstNode *base = node->b; base != NULL && !failed; base = base->next) {
        if (base->kind != AST_BASE) {
            error("not a base");
            break;
        }
        const AstNode *base_name = base->b != NULL ? base->b : base->a;
        const char *base_text = node_name(base_name);
        Record *base_rec = class_named(base_text);
        if (base_rec == NULL || !base_rec->complete) {
            error_name("base is incomplete", base_text == NULL ? "" : base_text);
            break;
        }
        std::string base_key = base_rec->name;
        Ty base_ty = record_type(*base_rec);
        if (base_key == key || derives_from(base_key, key)) {
            error("recursive base");
            break;
        }
        bool virtual_base = base->text != NULL && std::strcmp(base->text, "virtual") == 0;
        Field field;
        field.name = base_field_name(base_key);
        field.ty = base_ty;
        field.access = base->b != NULL && base->a != NULL && base->a->text != NULL
            ? base->a->text : "private";
        if (virtual_base) {
            records[key].direct_virtual.push_back(base_key);
        } else {
            records[key].bases.push_back(base_key);
            records[key].fields.push_back(field);
        }
    }
    {
        std::vector<std::string> order;
        std::set<std::string> seen;
        collect_vbases(key, order, seen);
        records[key].vbases = std::move(order);
    }
    struct Pending {
        MethodDecl decl;
        Ty ret;
        std::vector<Param> params;
        const AstNode *body = NULL;
        const AstNode *inits = NULL;
    };
    std::vector<Pending> pending;
    std::string access = "private";
    for (const AstNode *mem = node->c; mem != NULL && !failed; mem = mem->next) {
        if (mem->kind == AST_ACCESS) {
            access = mem->text != NULL ? mem->text : "private";
            continue;
        }
        if (mem->kind == AST_CONSTRUCTOR || mem->kind == AST_DESTRUCTOR) {
            Pending one;
            one.decl.access = access;
            one.decl.is_ctor = mem->kind == AST_CONSTRUCTOR;
            one.decl.is_dtor = mem->kind == AST_DESTRUCTOR;
            one.decl.is_virtual = one.decl.is_dtor && mem->b != NULL && mem->b->text != NULL
                && std::strcmp(mem->b->text, "virtual") == 0;
            one.decl.name = one.decl.is_dtor ? std::string("~") + plain : plain;
            one.decl.ir_name = one.decl.is_dtor ? source + "::~" + plain : source + "::" + plain;
            one.ret = void_ty();
            one.body = one.decl.is_dtor ? mem->a : mem->c;
            one.inits = one.decl.is_ctor ? mem->b : NULL;
            bool vararg = false;
            if (one.decl.is_ctor && !collect_params(mem->a, one.params, vararg))
                break;
            if (vararg) {
                error("variadic method is not in this phase");
                break;
            }
            pending.push_back(std::move(one));
            continue;
        }
        if (mem->kind != AST_METHOD && mem->kind != AST_DECLARATION) {
            error("not a member");
            break;
        }
        const AstNode *specs = mem->kind == AST_METHOD ? mem->a : mem->a;
        const AstNode *declarators = mem->kind == AST_METHOD ? mem->b : mem->b;
        Storage storage;
        Ty base_ty = parse_specs(specs, Where::Param, &storage);
        if (failed)
            break;
        if (storage.is_typedef || storage.is_extern || storage.is_static || storage.is_register) {
            error("storage class is not allowed here");
            break;
        }
        if (mem->kind == AST_DECLARATION) {
            if (declarators == NULL) {
                error("member has no name");
                break;
            }
            for (const AstNode *item = declarators; item != NULL && !failed; item = item->next) {
                Declared declared = declare_one(base_ty, item);
                if (failed)
                    break;
                if (declared.func != NULL) {
                    Pending one;
                    bool is_const = false;
                    if (!quals_const(declared.func->c, is_const))
                        break;
                    bool vararg = false;
                    if (!collect_params(declared.func->b, one.params, vararg))
                        break;
                    if (vararg || declared.name == NULL || declared.name->text == NULL) {
                        error("not a method");
                        break;
                    }
                    one.decl.name = declared.name->text;
                    one.decl.ir_name = source + "::" + one.decl.name;
                    one.decl.access = access;
                    one.decl.is_const = is_const;
                    one.decl.is_virtual = storage.is_virtual;
                    one.decl.is_pure = declared.func->d != NULL && declared.func->d->text != NULL
                        && std::strcmp(declared.func->d->text, "pure") == 0;
                    if (one.decl.is_pure)
                        one.decl.is_virtual = true;
                    one.ret = drop(declared.ty);
                    pending.push_back(std::move(one));
                    continue;
                }
                if (storage.is_virtual) {
                    error("virtual is not allowed here");
                    break;
                }
                if (declared.name == NULL || declared.name->kind != AST_IDENT
                    || declared.name->text == NULL) {
                    error("member has no name");
                    break;
                }
                if (!complete_type(declared.ty)) {
                    error("incomplete member");
                    break;
                }
                Field field;
                field.name = declared.name->text;
                field.ty = declared.ty;
                field.access = access;
                for (const Field &earlier : records[key].fields) {
                    if (earlier.name == field.name)
                        error_name("name is already defined", field.name.c_str());
                }
                if (failed)
                    break;
                records[key].fields.push_back(field);
            }
            continue;
        }
        Declared declared = declare_one(base_ty, declarators);
        if (failed)
            break;
        if (declared.func == NULL || declared.name == NULL || declared.name->kind != AST_IDENT
            || declared.name->text == NULL) {
            error("not a method");
            break;
        }
        Pending one;
        if (!quals_const(declared.func->c, one.decl.is_const))
            break;
        bool vararg = false;
        if (!collect_params(declared.func->b, one.params, vararg))
            break;
        if (vararg) {
            error("variadic method is not in this phase");
            break;
        }
        one.decl.name = declared.name->text;
        one.decl.ir_name = source + "::" + one.decl.name;
        one.decl.access = access;
        one.decl.is_virtual = storage.is_virtual;
        one.ret = drop(declared.ty);
        one.body = mem->c;
        pending.push_back(std::move(one));
    }
    if (failed)
        return int_ty();
    bool own_virtual = false;
    for (Pending &one : pending) {
        if (one.decl.is_dtor && !one.decl.is_virtual) {
            for (const std::string &base : records[key].bases) {
                if (virtual_destructor(base))
                    one.decl.is_virtual = true;
            }
            for (const std::string &base : records[key].direct_virtual) {
                if (virtual_destructor(base))
                    one.decl.is_virtual = true;
            }
        }
        if (one.decl.is_dtor && one.decl.is_virtual)
            own_virtual = true;
        if (one.decl.is_ctor || one.decl.is_dtor)
            continue;
        std::string vkey = virtual_key(one.ret, one.params, one.decl.is_const);
        bool inherited = false;
        for (const std::string &base : records[key].bases) {
            if (hides_virtual(base, one.decl.name, vkey))
                inherited = true;
        }
        for (const std::string &base : records[key].direct_virtual) {
            if (hides_virtual(base, one.decl.name, vkey))
                inherited = true;
        }
        if (inherited)
            one.decl.is_virtual = true;
        if (one.decl.is_virtual)
            own_virtual = true;
    }
    bool share_vptr = !records[key].bases.empty() && vptr_at_start(records[key].bases[0]);
    if (own_virtual && !share_vptr) {
        Field vptr;
        vptr.name = ".vptr";
        vptr.ty = pointer_to(int_ty());
        vptr.access = "private";
        records[key].fields.insert(records[key].fields.begin(), vptr);
        records[key].vptr = 0;
    }
    if (!records[key].vbases.empty()) {
        Field vbptr;
        vbptr.name = ".vbptr";
        vbptr.ty = pointer_to(int_ty());
        vbptr.access = "private";
        records[key].fields.push_back(vbptr);
        records[key].vbptr = static_cast<int>(records[key].fields.size() - 1);
    }
    layout_class(records[key]);
    place_vbases(key);
    records[key].methods.clear();
    for (const Pending &one : pending)
        install_method(key, one.decl, one.ret, one.params, one.body, one.inits);
    if (!failed)
        build_vtable(key);
    return failed ? int_ty() : record_type(records[key]);
}

std::optional<Slot> Codegen::member_slot(llvm::Value *object, const Ty &type, const char *name, bool report)
{
    if (!type.is_record || name == NULL) {
        if (report)
            error("not a class or struct");
        return std::nullopt;
    }
    auto found = records.find(type.record);
    if (found == records.end() || !found->second.complete) {
        if (report)
            error("incomplete type");
        return std::nullopt;
    }
    Record &rec = found->second;
    for (const Field &field : rec.fields) {
        if (field.name != name)
            continue;
        if (!can_access(field.access, type.record)) {
            error("cannot access");
            return std::nullopt;
        }
        Slot slot = field_addr(object, type, field);
        if (type.is_const)
            slot.ty.is_const = true;
        return slot;
    }
    for (const std::string &base : rec.bases) {
        std::string field_name = base_field_name(base);
        const Field *base_field = NULL;
        for (const Field &field : rec.fields) {
            if (field.name == field_name)
                base_field = &field;
        }
        if (base_field == NULL)
            continue;
        Slot base_slot = field_addr(object, type, *base_field);
        if (type.is_const)
            base_slot.ty.is_const = true;
        std::optional<Slot> nested = member_slot(base_slot.addr, base_slot.ty, name, false);
        if (nested || failed)
            return nested;
    }
    for (const std::string &base : rec.vbases) {
        auto base_rec = records.find(base);
        if (base_rec == records.end())
            continue;
        llvm::Value *addr = vbase_addr(object, type.record, base);
        Ty base_ty = record_type(base_rec->second);
        if (type.is_const)
            base_ty.is_const = true;
        std::optional<Slot> nested = member_slot(addr, base_ty, name, false);
        if (nested || failed)
            return nested;
    }
    if (report)
        error_name("no such member", name);
    return std::nullopt;
}

std::optional<Ty> Codegen::member_type(const Ty &type, const char *name)
{
    if (!type.is_record || name == NULL)
        return std::nullopt;
    auto found = records.find(type.record);
    if (found == records.end() || !found->second.complete)
        return std::nullopt;
    for (const Field &field : found->second.fields) {
        if (field.name == name) {
            if (!can_access(field.access, type.record)) {
                error("cannot access");
                return std::nullopt;
            }
            Ty ty = field.ty;
            if (type.is_const)
                ty.is_const = true;
            return ty;
        }
    }
    for (const std::string &base : found->second.bases) {
        auto base_rec = records.find(base);
        if (base_rec == records.end())
            continue;
        Ty base_ty = record_type(base_rec->second);
        if (type.is_const)
            base_ty.is_const = true;
        std::optional<Ty> nested = member_type(base_ty, name);
        if (nested || failed)
            return nested;
    }
    for (const std::string &base : found->second.vbases) {
        auto base_rec = records.find(base);
        if (base_rec == records.end())
            continue;
        Ty base_ty = record_type(base_rec->second);
        if (type.is_const)
            base_ty.is_const = true;
        std::optional<Ty> nested = member_type(base_ty, name);
        if (nested || failed)
            return nested;
    }
    return std::nullopt;
}

std::optional<Ty> Codegen::implicit_member_type(const char *name)
{
    if (method_class.empty() || name == NULL)
        return std::nullopt;
    auto found = records.find(method_class);
    if (found == records.end())
        return std::nullopt;
    Ty ty = record_type(found->second);
    if (method_const)
        ty.is_const = true;
    return member_type(ty, name);
}

std::optional<Slot> Codegen::implicit_member(const char *name)
{
    if (method_class.empty() || name == NULL || function == NULL)
        return std::nullopt;
    std::optional<Slot> self = lookup("this");
    if (!self)
        return std::nullopt;
    RValue ptr = read_slot(*self);
    if (failed || !ptr.ty.is_pointer || ptr.ty.inner == NULL)
        return std::nullopt;
    return member_slot(ptr.v, *ptr.ty.inner, name, false);
}

std::string Codegen::method_owner(const std::string &klass, const char *name, std::string &ir_name)
{
    if (klass.empty() || name == NULL)
        return "";
    auto found = records.find(klass);
    if (found == records.end())
        return "";
    std::string candidate = source_name(found->second) + "::" + name;
    if (overloads.count(candidate) != 0) {
        ir_name = candidate;
        return klass;
    }
    for (const std::string &base : found->second.bases) {
        std::string owner = method_owner(base, name, ir_name);
        if (!owner.empty())
            return owner;
    }
    for (const std::string &base : found->second.direct_virtual) {
        std::string owner = method_owner(base, name, ir_name);
        if (!owner.empty())
            return owner;
    }
    return "";
}

bool Codegen::prepare_method(const AstNode *callee, MethodCall &call)
{
    if (callee == NULL)
        return false;
    if (callee->kind == AST_IDENT) {
        std::string ir_name;
        std::string owner = method_owner(method_class, callee->text, ir_name);
        if (owner.empty())
            return false;
        call.ir_name = ir_name;
        call.owner = owner;
        call.implicit = true;
        return true;
    }
    if (callee->kind == AST_QUALIFIED) {
        std::vector<const AstNode *> parts;
        flatten_name(callee, parts);
        if (parts.size() < 2 || parts[0] == NULL || parts.back() == NULL || parts.back()->text == NULL)
            return false;
        Record *rec = class_named(node_name(parts[0]));
        if (rec == NULL)
            return false;
        if (parts.size() != 2) {
            error("not in this phase");
            return false;
        }
        std::string ir_name;
        std::string owner = method_owner(rec->name, parts.back()->text, ir_name);
        if (owner.empty()) {
            error_name("not a function", parts.back()->text);
            return false;
        }
        call.ir_name = ir_name;
        call.owner = owner;
        call.qualified = true;
        call.implicit = true;
        return true;
    }
    if (callee->kind != AST_ARROW && callee->kind != AST_MEMBER)
        return false;
    Ty obj = callee->kind == AST_ARROW ? decay(type_of(callee->a)) : object_type(callee->a);
    if (failed)
        return false;
    if (callee->kind == AST_ARROW) {
        if (!obj.is_pointer || obj.inner == NULL) {
            error("not a pointer");
            return false;
        }
        obj = *obj.inner;
    }
    if (!class_type(obj)) {
        error_name("not a function", callee->text);
        return false;
    }
    std::string ir_name;
    std::string owner = method_owner(obj.record, callee->text, ir_name);
    if (owner.empty()) {
        error_name("not a function", callee->text);
        return false;
    }
    call.ir_name = ir_name;
    call.owner = owner;
    call.object = callee->a;
    call.arrow = callee->kind == AST_ARROW;
    return true;
}

llvm::Value *Codegen::adjust_to(llvm::Value *ptr, const std::string &from, const std::string &owner)
{
    if (failed || from == owner || from.empty())
        return ptr;
    auto found = records.find(from);
    if (found == records.end()) {
        error("not a base");
        return ptr;
    }
    for (const std::string &base : found->second.bases) {
        if (base != owner && !derives_from(base, owner))
            continue;
        const Field *field = NULL;
        for (const Field &one : found->second.fields) {
            if (one.name == base_field_name(base))
                field = &one;
        }
        if (field == NULL)
            break;
        Slot slot = field_addr(ptr, record_type(found->second), *field);
        return adjust_to(slot.addr, base, owner);
    }
    for (const std::string &base : found->second.vbases) {
        if (base == owner || derives_from(base, owner))
            return adjust_to(vbase_addr(ptr, from, base), base, owner);
    }
    error("not a base");
    return ptr;
}

const MethodDecl *Codegen::method_decl(const std::string &owner, const std::string &ir_name) const
{
    auto found = records.find(owner);
    if (found == records.end())
        return NULL;
    for (const MethodDecl &decl : found->second.methods) {
        if (decl.ir_name == ir_name && !decl.is_ctor && !decl.is_dtor)
            return &decl;
    }
    return NULL;
}

Func *Codegen::pick_method(const MethodCall &call, const std::vector<Ty> &args, bool object_const, const char *label)
{
    Ty self = pointer_to(record_type(records[call.owner]));
    if (object_const && self.inner != NULL)
        self.inner->is_const = true;
    std::vector<Ty> full = args;
    full.insert(full.begin(), self);
    Func *fn = resolve_keys({call.ir_name}, full, label);
    if (fn == NULL)
        return NULL;
    if (object_const && !fn->method_const) {
        error("const method");
        return NULL;
    }
    const MethodDecl *decl = method_decl(call.owner, call.ir_name);
    if (decl != NULL && !can_access(decl->access, call.owner)) {
        error("cannot access");
        return NULL;
    }
    return fn;
}

Ty Codegen::type_method_call(const AstNode *node, const MethodCall &call)
{
    std::vector<Ty> args;
    for (const AstNode *arg = node->b; arg != NULL && !failed; arg = arg->next)
        args.push_back(decay(type_of(arg)));
    if (failed)
        return int_ty();
    bool object_const = call.implicit ? method_const : false;
    if (!call.implicit && call.object != NULL) {
        Ty obj = call.arrow ? decay(type_of(call.object)) : object_type(call.object);
        if (call.arrow && obj.is_pointer && obj.inner != NULL)
            object_const = obj.inner->is_const;
        else
            object_const = obj.is_const;
    }
    Func *fn = pick_method(call, args, object_const, call.ir_name.c_str());
    if (fn == NULL)
        return int_ty();
    return drop(fn->ret);
}

RValue Codegen::emit_method_call(const AstNode *node, const MethodCall &call)
{
    std::vector<RValue> args;
    for (const AstNode *arg = node->b; arg != NULL && !failed; arg = arg->next)
        args.push_back(emit_rvalue(arg));
    if (failed)
        return fallback();
    llvm::Value *self = NULL;
    std::string from;
    bool object_const = false;
    if (call.implicit) {
        std::optional<Slot> slot = lookup("this");
        if (!slot) {
            error("this is not allowed here");
            return fallback();
        }
        RValue ptr = read_slot(*slot);
        self = ptr.v;
        from = method_class;
        object_const = method_const;
    } else if (call.arrow) {
        RValue ptr = emit_rvalue(call.object);
        if (failed)
            return fallback();
        if (!ptr.ty.is_pointer || ptr.ty.inner == NULL) {
            error("not a pointer");
            return fallback();
        }
        self = ptr.v;
        from = ptr.ty.inner->record;
        object_const = ptr.ty.inner->is_const;
    } else {
        std::optional<Slot> slot = lvalue(call.object);
        if (!slot)
            return fallback();
        self = slot->addr;
        from = slot->ty.record;
        object_const = slot->ty.is_const;
    }
    self = adjust_to(self, from, call.owner);
    if (failed)
        return fallback();
    std::vector<Ty> types;
    for (const RValue &arg : args)
        types.push_back(arg.ty);
    const char *label = call.ir_name.c_str();
    Func *target = pick_method(call, types, object_const, label);
    if (target == NULL || failed)
        return fallback();
    std::vector<llvm::Value *> values;
    values.push_back(self);
    unsigned user = static_cast<unsigned>(target->params.size() - 1);
    unsigned provided = static_cast<unsigned>(args.size());
    unsigned limit = provided < user ? provided : user;
    for (unsigned i = 0; i < limit; i++)
        values.push_back(convert(args[i].v, args[i].ty, target->params[i + 1]));
    for (unsigned i = limit; i < user; i++)
        values.push_back(target->defaults[i + 1]);
    llvm::Value *callee = target->fn;
    if (!call.qualified && target->vindex >= 0 && records.count(call.owner) != 0) {
        auto *ptr_ty = llvm::PointerType::get(context, 0);
        llvm::Value *table = builder.CreateLoad(ptr_ty, vptr_addr(self, call.owner));
        llvm::Value *index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                                    static_cast<uint64_t>(target->vindex));
        llvm::Value *slot = builder.CreateInBoundsGEP(ptr_ty, table, {index});
        callee = builder.CreateLoad(ptr_ty, slot);
    }
    if (failed)
        return fallback();
    llvm::Value *result = builder.CreateCall(target->fn->getFunctionType(), callee, values);
    RValue out;
    out.ty = drop(target->ret);
    out.v = out.ty.is_void ? nullptr : result;
    return out;
}

void Codegen::declare_special(const AstNode *node)
{
    const char *plain = node->text;
    Record *rec = class_named(plain);
    if (rec == NULL || !rec->complete) {
        error_name("unknown class", plain == NULL ? "" : plain);
        return;
    }
    std::string key = rec->name;
    std::string source = source_name(*rec);
    MethodDecl decl;
    decl.is_ctor = node->kind == AST_CONSTRUCTOR;
    decl.is_dtor = node->kind == AST_DESTRUCTOR;
    decl.name = decl.is_dtor ? std::string("~") + plain : plain;
    decl.ir_name = decl.is_dtor ? source + "::~" + plain : source + "::" + plain;
    decl.access = "public";
    for (const MethodDecl &existing : records[key].methods) {
        if (existing.ir_name == decl.ir_name)
            decl.access = existing.access;
    }
    std::vector<Param> params;
    bool vararg = false;
    if (decl.is_ctor && !collect_params(node->a, params, vararg))
        return;
    if (vararg) {
        error("variadic method is not in this phase");
        return;
    }
    const AstNode *body = decl.is_dtor ? node->a : node->c;
    const AstNode *inits = decl.is_ctor ? node->b : NULL;
    install_method(key, decl, void_ty(), std::move(params), body, inits);
}

void Codegen::declare_qualified_method(const Declared &declared, bool is_const, bool is_virtual,
                              const AstNode *body)
{
    std::vector<const AstNode *> parts;
    flatten_name(declared.name, parts);
    if (parts.size() != 2 || parts[0] == NULL || parts.back() == NULL || parts.back()->text == NULL) {
        error("not in this phase");
        return;
    }
    Record *rec = class_named(node_name(parts[0]));
    if (rec == NULL || !rec->complete) {
        error_name("unknown class", node_name(parts[0]) == NULL ? "" : node_name(parts[0]));
        return;
    }
    std::string key = rec->name;
    MethodDecl decl;
    decl.name = parts.back()->text;
    decl.ir_name = source_name(*rec) + "::" + decl.name;
    decl.is_const = is_const;
    decl.is_virtual = is_virtual;
    decl.access = "public";
    bool known = false;
    for (const MethodDecl &existing : records[key].methods) {
        if (existing.ir_name == decl.ir_name && existing.is_const == is_const) {
            decl.access = existing.access;
            decl.is_virtual = existing.is_virtual || is_virtual;
            known = true;
        }
    }
    if (!known) {
        error_name("not a member", decl.name.c_str());
        return;
    }
    std::vector<Param> params;
    bool vararg = false;
    if (!collect_params(declared.func->b, params, vararg))
        return;
    if (vararg) {
        error("variadic method is not in this phase");
        return;
    }
    install_method(key, decl, drop(declared.ty), std::move(params), body, NULL);
}

void Codegen::emit_ctor_call(llvm::Value *self, const std::string &owner, const AstNode *args,
                             bool most_derived)
{
    std::string plain = source_name(records[owner]);
    auto pos = plain.rfind("::");
    std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
    std::string ir_name = plain + "::" + last;
    std::vector<RValue> values;
    for (const AstNode *arg = args; arg != NULL && !failed; arg = arg->next)
        values.push_back(emit_rvalue(arg));
    if (failed)
        return;
    std::vector<Ty> types;
    types.push_back(pointer_to(record_type(records[owner])));
    for (const RValue &value : values)
        types.push_back(value.ty);
    Func *target = resolve_keys({ir_name}, types, last.c_str());
    if (target == NULL || failed)
        return;
    std::vector<llvm::Value *> operands;
    operands.push_back(self);
    unsigned user = static_cast<unsigned>(target->params.size() - 1);
    unsigned provided = static_cast<unsigned>(values.size());
    unsigned limit = provided < user ? provided : user;
    for (unsigned i = 0; i < limit; i++)
        operands.push_back(convert(values[i].v, values[i].ty, target->params[i + 1]));
    for (unsigned i = limit; i < user; i++) {
        if (i + 1 < target->names.size() && target->names[i + 1] == ".most")
            operands.push_back(ctor_flag(most_derived));
        else
            operands.push_back(target->defaults[i + 1]);
    }
    builder.CreateCall(target->fn->getFunctionType(), target->fn, operands);
}

void Codegen::emit_inits(Func &fn)
{
    Record &rec = records[fn.klass];
    std::optional<Slot> self = lookup("this");
    if (!self) {
        error("this is not allowed here");
        return;
    }
    RValue ptr = read_slot(*self);
    if (failed || !ptr.ty.is_pointer || ptr.ty.inner == NULL)
        return;
    if (!rec.vbases.empty()) {
        std::optional<Slot> most = lookup(".most");
        llvm::Value *flag = most ? load_slot(*most) : ctor_flag(true);
        llvm::BasicBlock *yes = new_block("vbases");
        llvm::BasicBlock *no = new_block("vbases.skip");
        llvm::BasicBlock *next = new_block("inits");
        llvm::Value *is_most = builder.CreateICmpNE(flag, ctor_flag(false));
        builder.CreateCondBr(is_most, yes, no);
        builder.SetInsertPoint(yes);
        emit_virtual_bases(ptr.v, fn.klass, fn.inits);
        builder.CreateBr(next);
        builder.SetInsertPoint(no);
        builder.CreateBr(next);
        builder.SetInsertPoint(next);
    }
    for (const AstNode *init = fn.inits; init != NULL && !failed; init = init->next) {
        if (init->kind != AST_MEM_INIT || init->text == NULL) {
            error("not an initializer");
            return;
        }
        std::string base_key;
        for (const std::string &base : rec.bases) {
            std::string plain = source_name(records[base]);
            auto pos = plain.rfind("::");
            std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
            if (last == init->text)
                base_key = base;
        }
        if (!base_key.empty()) {
            llvm::Value *sub = adjust_to(ptr.v, fn.klass, base_key);
            if (!failed)
                emit_ctor_call(sub, base_key, init->a, false);
            continue;
        }
        bool virtual_init = false;
        for (const std::string &base : rec.vbases) {
            std::string plain = source_name(records[base]);
            auto pos = plain.rfind("::");
            std::string last = pos == std::string::npos ? plain : plain.substr(pos + 2);
            if (last == init->text)
                virtual_init = true;
        }
        if (virtual_init)
            continue;
        std::optional<Slot> field = member_slot(ptr.v, *ptr.ty.inner, init->text, true);
        if (!field)
            return;
        if (class_type(field->ty)) {
            emit_ctor_call(field->addr, field->ty.record, init->a, false);
            continue;
        }
        if (init->a == NULL || init->a->next != NULL) {
            error("initializer argument");
            return;
        }
        RValue value = emit_rvalue(init->a);
        if (!failed)
            store_slot(*field, convert(value.v, value.ty, field->ty), true);
    }
    if (!failed)
        emit_vptrs(ptr.v, fn.klass);
}
