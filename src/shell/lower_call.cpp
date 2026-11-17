/*
 * Calls, overloads, and string literals.
 */
#include "lower.hpp"

bool Codegen::is_string(const AstNode *node)
{
    return node != NULL && node->kind == AST_LITERAL && node->text != NULL
        && (node->text[0] == '"' || (node->text[0] == 'L' && node->text[1] == '"'));
}

Ty Codegen::string_ty()
{
    Ty ty = pointer_to(integer(8, true, 1));
    return ty;
}

bool Codegen::decode_string(const char *text, std::string &bytes)
{
    if (text == NULL)
        return false;
    if (text[0] == 'L') {
        error("wide strings are not in this phase");
        return false;
    }
    if (text[0] != '"')
        return false;
    bytes.clear();
    for (std::size_t i = 1; text[i] != '\0' && text[i] != '"'; i++) {
        if (text[i] != '\\') {
            bytes.push_back(text[i]);
            continue;
        }
        char escaped = text[++i];
        if (escaped == 'n')
            bytes.push_back('\n');
        else if (escaped == 't')
            bytes.push_back('\t');
        else if (escaped == 'r')
            bytes.push_back('\r');
        else if (escaped == '0')
            bytes.push_back('\0');
        else if (escaped == '\\' || escaped == '"' || escaped == '\'')
            bytes.push_back(escaped);
        else {
            error("bad string escape");
            return false;
        }
    }
    return !failed;
}

RValue Codegen::emit_string(const char *text)
{
    std::string bytes;
    if (!decode_string(text, bytes)) {
        if (!failed)
            error("bad string");
        return fallback();
    }
    llvm::Constant *init = llvm::ConstantDataArray::getString(context, bytes, true);
    auto *global = new llvm::GlobalVariable(
        module, init->getType(), true, llvm::GlobalValue::PrivateLinkage, init, ".str");
    llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
    RValue out;
    out.ty = string_ty();
    out.v = builder.CreateInBoundsGEP(init->getType(), global, {zero, zero}, "str");
    return out;
}

bool Codegen::same_value_type(Ty left, Ty right)
{
    left = drop(left);
    right = drop(right);
    if (left.is_pointer || right.is_pointer) {
        if (!left.is_pointer || !right.is_pointer || left.inner == NULL || right.inner == NULL)
            return false;
        if (left.is_null || right.is_null)
            return true;
        return same_value_type(*left.inner, *right.inner);
    }
    if (left.is_array || right.is_array) {
        return left.is_array && right.is_array && left.array_len == right.array_len
            && left.inner != NULL && right.inner != NULL
            && same_value_type(*left.inner, *right.inner);
    }
    if (left.is_record || right.is_record)
        return left.is_record && right.is_record && left.record == right.record;
    return left.ir == right.ir && left.is_signed == right.is_signed
        && left.is_float == right.is_float && left.is_bool == right.is_bool
        && left.is_void == right.is_void;
}

bool Codegen::storage_conflict(const Storage &storage)
{
    int classes = static_cast<int>(storage.is_typedef) + static_cast<int>(storage.is_extern)
        + static_cast<int>(storage.is_static) + static_cast<int>(storage.is_register);
    if (classes > 1)
        return true;
    return storage.is_inline && (storage.is_typedef || storage.is_register);
}

std::string Codegen::type_tag(const Ty &ty)
{
    if (ty.is_void)
        return "void";
    if (ty.is_bool)
        return "bool";
    if (ty.is_pointer && ty.inner != NULL)
        return type_tag(*ty.inner) + "*";
    if (ty.is_array && ty.inner != NULL)
        return type_tag(*ty.inner) + "[" + std::to_string(ty.array_len) + "]";
    if (ty.is_record)
        return ty.record;
    if (ty.is_float)
        return ty.fp_rank >= 2 ? "double" : "float";
    return std::string("i") + std::to_string(ty.ir->getIntegerBitWidth())
        + (ty.is_signed ? "s" : "u");
}

std::string Codegen::sig_text(const Func &fn)
{
    std::string text = fn.source + "(";
    for (std::size_t i = 0; i < fn.params.size(); i++) {
        if (i != 0)
            text += ",";
        text += type_tag(fn.params[i]);
    }
    if (fn.vararg) {
        if (!fn.params.empty())
            text += ",";
        text += "...";
    }
    text += ")";
    return text;
}

void Codegen::rename_group(std::vector<Func> &group)
{
    for (Func &fn : group)
        fn.fn->setName(sig_text(fn));
}

bool Codegen::same_signature(const Func &left, const Func &right)
{
    if (left.vararg != right.vararg || left.params.size() != right.params.size())
        return false;
    for (std::size_t i = 0; i < left.params.size(); i++) {
        if (!same_value_type(left.params[i], right.params[i]))
            return false;
    }
    return true;
}

void Codegen::install(Func incoming)
{
    if (scopes.front().count(incoming.source) != 0 || aliases.count(incoming.source) != 0
        || enum_consts.count(incoming.source) != 0 || namespaces.count(incoming.source) != 0) {
        error_name("name is already defined", incoming.source.c_str());
        return;
    }
    auto &group = overloads[incoming.source];
    for (Func &existing : group) {
        if (!same_signature(existing, incoming))
            continue;
        if (!same_value_type(existing.ret, incoming.ret)) {
            error_name("incompatible return type", incoming.source.c_str());
            return;
        }
        if (existing.is_static != incoming.is_static) {
            error_name("incompatible linkage", incoming.source.c_str());
            return;
        }
        if (existing.body != NULL && incoming.body != NULL) {
            error_name("name is already defined", incoming.source.c_str());
            return;
        }
        if (incoming.body != NULL) {
            existing.body = incoming.body;
            existing.names = incoming.names;
            existing.inits = incoming.inits;
            if (!incoming.klass.empty()) {
                existing.klass = incoming.klass;
                existing.method_const = incoming.method_const;
                existing.is_ctor = incoming.is_ctor;
                existing.is_dtor = incoming.is_dtor;
            }
        }
        if (incoming.is_inline && !existing.is_inline) {
            existing.is_inline = true;
            existing.fn->addFnAttr(llvm::Attribute::InlineHint);
        }
        for (std::size_t i = 0; i < existing.defaults.size(); i++) {
            if (existing.defaults[i] == NULL && incoming.defaults[i] != NULL)
                existing.defaults[i] = incoming.defaults[i];
        }
        return;
    }
    std::vector<llvm::Type *> arg_types;
    for (const Ty &ty : incoming.params)
        arg_types.push_back(ty.ir);
    auto *type = llvm::FunctionType::get(incoming.ret.ir, arg_types, incoming.vararg);
    std::string ir_name = group.empty() ? incoming.source : sig_text(incoming);
    auto linkage = incoming.is_static ? llvm::GlobalValue::InternalLinkage
                                      : llvm::GlobalValue::ExternalLinkage;
    incoming.fn = llvm::Function::Create(type, linkage, ir_name, &module);
    if (incoming.is_inline)
        incoming.fn->addFnAttr(llvm::Attribute::InlineHint);
    group.push_back(std::move(incoming));
    if (group.size() > 1)
        rename_group(group);
}

Func *Codegen::resolve_keys(const std::vector<std::string> &keys, const std::vector<Ty> &args, const char *label)
{
    bool any = false;
    for (const std::string &key : keys) {
        if (namespace_only(key)) {
            error("namespace is not a value");
            return NULL;
        }
        if (overloads.count(key) != 0)
            any = true;
    }
    if (!any) {
        error_name("unknown function", label);
        return NULL;
    }
    Func *best = NULL;
    int best_rank = -1;
    int best_filled = 0;
    int ties = 0;
    for (const std::string &key : keys) {
        auto found = overloads.find(key);
        if (found == overloads.end())
            continue;
        for (Func &fn : found->second) {
        unsigned required = 0;
        while (required < fn.params.size() && fn.defaults[required] == NULL)
            required++;
        if (args.size() < required)
            continue;
        if (!fn.vararg && args.size() > fn.params.size())
            continue;
        int rank = 0;
        bool viable = true;
        unsigned limit = static_cast<unsigned>(std::min(args.size(), fn.params.size()));
        for (unsigned i = 0; i < limit; i++) {
            if (args[i].is_void) {
                viable = false;
                break;
            }
            bool aggregate = args[i].is_pointer || fn.params[i].is_pointer || args[i].is_record
                || fn.params[i].is_record || args[i].is_array || fn.params[i].is_array;
            if (aggregate && !same_value_type(args[i], fn.params[i])) {
                viable = false;
                break;
            }
            rank += same_value_type(args[i], fn.params[i]) ? 2 : 1;
        }
        if (!viable)
            continue;
        int filled = args.size() < fn.params.size()
            ? static_cast<int>(fn.params.size() - args.size()) : 0;
        if (best == NULL || rank > best_rank || (rank == best_rank && filled < best_filled)) {
            best = &fn;
            best_rank = rank;
            best_filled = filled;
            ties = 1;
        } else if (rank == best_rank && filled == best_filled)
            ties++;
        }
    }
    if (best == NULL) {
        error_name("no matching function", label);
        return NULL;
    }
    if (ties != 1) {
        error_name("ambiguous call", label);
        return NULL;
    }
    return best;
}

Func *Codegen::resolve(const char *name, const std::vector<Ty> &args)
{
    if (name == NULL) {
        error("the call has no name");
        return NULL;
    }
    if (lookup(name)) {
        error_name("not a function", name);
        return NULL;
    }
    return resolve_keys(visible_keys(name), args, name);
}

Ty Codegen::vararg_ty(Ty ty)
{
    if (ty.is_float && ty.fp_rank < 2)
        return floating(true);
    return promote(ty);
}

const AstNode *Codegen::function_declarator(const AstNode *node)
{
    if (node != NULL && node->kind == AST_INIT)
        node = node->a;
    if (node != NULL && node->kind == AST_FUNCTION_DECL)
        return node;
    return NULL;
}

bool Codegen::call_target(const AstNode *callee, std::vector<std::string> &keys, const char *&label)
{
    if (callee == NULL) {
        error("the call has no name");
        return false;
    }
    if (callee->kind == AST_IDENT) {
        label = callee->text;
        if (label != NULL && lookup(label)) {
            error_name("not a function", label);
            return false;
        }
        keys = visible_keys(label == NULL ? "" : label);
        return true;
    }
    if (callee->kind == AST_QUALIFIED) {
        std::string key = entity_key(callee);
        if (failed)
            return false;
        label = callee->b != NULL && callee->b->text != NULL ? callee->b->text : "call";
        if (scopes[0].count(key) != 0) {
            error_name("not a function", label);
            return false;
        }
        keys.assign(1, key);
        return true;
    }
    error("calls of that form are a later phase");
    return false;
}

Ty Codegen::type_call(const AstNode *node)
{
    MethodCall method;
    if (prepare_method(node->a, method))
        return type_method_call(node, method);
    if (failed)
        return int_ty();
    std::vector<std::string> keys;
    const char *label = "call";
    if (!call_target(node->a, keys, label))
        return int_ty();
    std::vector<Ty> args;
    for (const AstNode *arg = node->b; arg != NULL && !failed; arg = arg->next)
        args.push_back(decay(type_of(arg)));
    if (failed)
        return int_ty();
    Func *fn = resolve_keys(keys, args, label);
    if (fn == NULL)
        return int_ty();
    return drop(fn->ret);
}

RValue Codegen::emit_call(const AstNode *node)
{
    MethodCall method;
    if (prepare_method(node->a, method))
        return emit_method_call(node, method);
    if (failed)
        return fallback();
    std::vector<std::string> keys;
    const char *label = "call";
    if (!call_target(node->a, keys, label))
        return fallback();
    std::vector<RValue> args;
    for (const AstNode *arg = node->b; arg != NULL && !failed; arg = arg->next)
        args.push_back(emit_rvalue(arg));
    if (failed)
        return fallback();
    std::vector<Ty> types;
    for (const RValue &arg : args)
        types.push_back(arg.ty);
    Func *target = resolve_keys(keys, types, label);
    if (target == NULL || failed)
        return fallback();
    std::vector<llvm::Value *> values;
    unsigned fixed = static_cast<unsigned>(target->params.size());
    unsigned provided = static_cast<unsigned>(args.size());
    unsigned limit = provided < fixed ? provided : fixed;
    for (unsigned i = 0; i < limit; i++)
        values.push_back(convert(args[i].v, args[i].ty, target->params[i]));
    for (unsigned i = limit; i < fixed; i++)
        values.push_back(target->defaults[i]);
    if (target->vararg) {
        for (unsigned i = fixed; i < provided; i++) {
            Ty widened = vararg_ty(args[i].ty);
            values.push_back(convert(args[i].v, args[i].ty, widened));
        }
    }
    if (failed)
        return fallback();
    llvm::Value *call = builder.CreateCall(target->fn->getFunctionType(), target->fn, values);
    RValue out;
    out.ty = drop(target->ret);
    out.v = out.ty.is_void ? nullptr : call;
    return out;
}

void Codegen::record_alias(const std::string &name, Ty ty)
{
    if (scopes.front().count(name) != 0 || overloads.count(name) != 0
        || enum_consts.count(name) != 0) {
        error_name("name is already defined", name.c_str());
        return;
    }
    auto found = aliases.find(name);
    if (found != aliases.end()) {
        Ty previous = found->second;
        if (!same_value_type(previous, ty) || previous.is_const != ty.is_const
            || previous.is_volatile != ty.is_volatile)
            error_name("incompatible typedef", name.c_str());
        return;
    }
    aliases[name] = ty;
}

bool Codegen::collect_params(const AstNode *list, std::vector<Param> &out, bool &vararg)
{
    vararg = false;
    if (list == NULL)
        return true;
    if (list->kind == AST_PARAMETER && list->next == NULL && list->b == NULL
        && list->c == NULL) {
        Storage storage;
        Ty ty = parse_specs(list->a, Where::Param, &storage);
        if (failed)
            return false;
        if (storage.is_typedef || storage.is_extern || storage.is_static || storage.is_inline) {
            error("storage class is not allowed here");
            return false;
        }
        if (ty.is_void && !ty.is_const && !ty.is_volatile)
            return true;
    }
    bool saw_default = false;
    for (const AstNode *node = list; node != NULL; node = node->next) {
        if (node->kind == AST_ELLIPSIS) {
            if (node->next != NULL) {
                error("ellipsis must be last");
                return false;
            }
            vararg = true;
            break;
        }
        if (node->kind != AST_PARAMETER) {
            error("not a parameter");
            return false;
        }
        Storage storage;
        Param param;
        param.ty = parse_specs(node->a, Where::Param, &storage);
        if (failed)
            return false;
        if (storage.is_typedef || storage.is_extern || storage.is_static || storage.is_inline) {
            error("storage class is not allowed here");
            return false;
        }
        if (node->b != NULL) {
            Declared declared = declare_one(param.ty, node->b);
            if (failed)
                return false;
            if (declared.func != NULL) {
                error("not in this phase");
                return false;
            }
            param.ty = declared.ty;
            if (declared.name != NULL) {
                if (declared.name->kind != AST_IDENT || declared.name->text == NULL) {
                    if (declared.name->kind == AST_QUALIFIED)
                        error("qualified names are a later phase");
                    else
                        error("the parameter has no name");
                    return false;
                }
                param.name = declared.name->text;
            }
        }
        if (param.ty.is_void) {
            error("void parameter");
            return false;
        }
        if (!complete_type(param.ty)) {
            error("incomplete type");
            return false;
        }
        if (node->c != NULL) {
            Const value = eval_const(node->c);
            if (!value.ok) {
                if (!failed)
                    error("default argument is not constant");
                return false;
            }
            param.fallback = llvm::cast<llvm::Constant>(
                convert_const(value.v, value.ty, param.ty));
            saw_default = true;
        } else if (saw_default) {
            error("default arguments must be trailing");
            return false;
        }
        out.push_back(param);
    }
    return !failed;
}

void Codegen::declare_function(const AstNode *specs, const AstNode *decl, const AstNode *body)
{
    Storage storage;
    Ty base = parse_specs(specs, Where::Param, &storage);
    if (failed)
        return;
    Declared declared = declare_one(base, decl);
    if (failed)
        return;
    if (declared.func == NULL) {
        error("not a function");
        return;
    }
    bool method_const_qual = false;
    if (declared.func->c != NULL && !quals_const(declared.func->c, method_const_qual))
        return;
    if (declared.func->c != NULL && (declared.name == NULL || declared.name->kind != AST_QUALIFIED)) {
        error("method qualifiers are not allowed here");
        return;
    }
    if (storage.is_virtual && (declared.name == NULL || declared.name->kind != AST_QUALIFIED)) {
        error("virtual is not allowed here");
        return;
    }
    if (body != NULL && body->kind != AST_COMPOUND) {
        error("function has no body");
        return;
    }
    if (declared.name != NULL && declared.name->kind == AST_QUALIFIED) {
        declare_qualified_method(declared, method_const_qual, storage.is_virtual, body);
        return;
    }
    if (declared.name == NULL || declared.name->kind != AST_IDENT || declared.name->text == NULL) {
        error("the function has no name");
        return;
    }
    if (storage_conflict(storage) || storage.is_typedef || storage.is_register) {
        error("storage class is not allowed here");
        return;
    }
    Ty ret = drop(declared.ty);
    if (ret.is_array) {
        error("function returns an array");
        return;
    }
    if (!ret.is_void && !complete_type(ret)) {
        error("incomplete type");
        return;
    }
    std::vector<Param> params;
    bool vararg = false;
    if (!collect_params(declared.func->b, params, vararg))
        return;
    Func incoming;
    incoming.source = qualify(declared.name->text);
    incoming.home = ns_stack;
    incoming.lookup = live_chain();
    incoming.ret = ret;
    incoming.vararg = vararg;
    incoming.body = body;
    incoming.is_static = storage.is_static;
    incoming.is_inline = storage.is_inline;
    for (const Param &param : params) {
        incoming.params.push_back(param.ty);
        incoming.defaults.push_back(param.fallback);
        incoming.names.push_back(param.name);
    }
    install(std::move(incoming));
}
