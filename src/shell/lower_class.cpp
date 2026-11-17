/*
 * Classes, methods, bases, and virtual calls.
 */
#include "lower.hpp"

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
    incoming.inits = inits;
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
    bool any_virtual = false;
    for (const MethodDecl &method : rec.methods) {
        if (method.is_virtual)
            any_virtual = true;
    }
    if (any_virtual) {
        Field vptr;
        vptr.name = ".vptr";
        vptr.ty = pointer_to(int_ty());
        vptr.access = "private";
        rec.fields.insert(rec.fields.begin(), vptr);
        rec.vptr = 0;
    }
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

void Codegen::build_vtable(const std::string &key)
{
    Record &rec = records[key];
    std::vector<llvm::Constant *> slots;
    int index = 0;
    for (MethodDecl &method : rec.methods) {
        if (!method.is_virtual)
            continue;
        auto found = overloads.find(method.ir_name);
        if (found == overloads.end())
            continue;
        for (Func &fn : found->second) {
            if (fn.klass != key || fn.fn == NULL || fn.vindex >= 0)
                continue;
            if (fn.method_const != method.is_const)
                continue;
            method.vindex = index;
            fn.vindex = index;
            slots.push_back(fn.fn);
            index++;
        }
    }
    if (slots.empty())
        return;
    auto *elem = llvm::PointerType::get(context, 0);
    auto *array = llvm::ArrayType::get(elem, slots.size());
    rec.vtable = new llvm::GlobalVariable(
        module, array, true, llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantArray::get(array, slots), rec.name + ".vt");
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
        Field field;
        field.name = base_field_name(base_key);
        field.ty = base_ty;
        field.access = base->b != NULL && base->a != NULL && base->a->text != NULL
            ? base->a->text : "private";
        records[key].bases.push_back(base_key);
        records[key].fields.push_back(field);
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
    for (const Pending &one : pending) {
        if (one.decl.is_virtual)
            records[key].methods.push_back(one.decl);
    }
    layout_class(records[key]);
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
    if (!call.qualified && target->vindex >= 0) {
        Record &owner = records[call.owner];
        if (owner.vptr >= 0) {
            Ty owner_ty = record_type(owner);
            Slot vslot = field_addr(self, owner_ty, owner.fields[static_cast<std::size_t>(owner.vptr)]);
            auto *ptr_ty = llvm::PointerType::get(context, 0);
            llvm::Value *table = builder.CreateLoad(ptr_ty, vslot.addr);
            llvm::Value *index = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context),
                                                        static_cast<uint64_t>(target->vindex));
            llvm::Value *slot = builder.CreateInBoundsGEP(ptr_ty, table, {index});
            callee = builder.CreateLoad(ptr_ty, slot);
        }
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

void Codegen::emit_ctor_call(llvm::Value *self, const std::string &owner, const AstNode *args)
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
    for (unsigned i = limit; i < user; i++)
        operands.push_back(target->defaults[i + 1]);
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
    if (rec.vtable != NULL && rec.vptr >= 0) {
        Slot vslot = field_addr(ptr.v, *ptr.ty.inner, rec.fields[static_cast<std::size_t>(rec.vptr)]);
        builder.CreateStore(rec.vtable, vslot.addr);
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
                emit_ctor_call(sub, base_key, init->a);
            continue;
        }
        std::optional<Slot> field = member_slot(ptr.v, *ptr.ty.inner, init->text, true);
        if (!field)
            return;
        if (class_type(field->ty)) {
            emit_ctor_call(field->addr, field->ty.record, init->a);
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
}
