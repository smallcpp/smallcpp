extern "C" {
#include "ast.h"
}

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

enum class Where { File, Local, Param, Return };

struct Ty {
    llvm::Type *ir = nullptr;
    bool is_signed = false;
    bool is_float = false;
    bool is_bool = false;
    bool is_void = false;
    bool is_const = false;
    bool is_volatile = false;
    bool is_pointer = false;
    bool is_array = false;
    bool is_record = false;
    bool is_union = false;
    bool is_enum = false;
    int array_len = 0;
    int rank = 0;
    int fp_rank = 0;
    std::string record;
    std::shared_ptr<Ty> inner;
};

struct Storage {
    bool is_typedef = false;
    bool is_extern = false;
    bool is_static = false;
    bool is_inline = false;
    bool is_register = false;
};

struct Slot {
    llvm::Value *addr = nullptr;
    Ty ty;
    int bits = 0;
};

struct Field {
    std::string name;
    Ty ty;
    int bits = 0;
    unsigned index = 0;
    llvm::Type *storage = nullptr;
};

struct Record {
    std::string name;
    bool is_union = false;
    bool is_class = false;
    bool complete = false;
    bool defining = false;
    llvm::StructType *ir = nullptr;
    std::vector<Field> fields;
};

struct RValue {
    llvm::Value *v = nullptr;
    Ty ty;
};

struct Const {
    bool ok = false;
    llvm::Constant *v = nullptr;
    Ty ty;
};

struct LoopFrame {
    llvm::BasicBlock *break_bb = nullptr;
    llvm::BasicBlock *continue_bb = nullptr;
};

struct CaseDest {
    llvm::ConstantInt *value = nullptr;
    llvm::BasicBlock *block = nullptr;
};

struct SwitchFrame {
    Ty ty;
    std::map<uint64_t, CaseDest> cases;
    llvm::BasicBlock *default_bb = nullptr;
    bool saw_default = false;
};

struct LabelInfo {
    llvm::BasicBlock *bb = nullptr;
    bool defined = false;
};

struct Param {
    std::string name;
    Ty ty;
    llvm::Constant *fallback = nullptr;
};

struct LookupScope {
    std::string ns;
    std::vector<std::pair<std::string, std::string>> names;
    std::vector<std::string> directives;
};

struct BlockUse {
    std::vector<std::pair<std::string, std::string>> names;
    std::vector<std::string> directives;
};

struct Func {
    std::string source;
    llvm::Function *fn = nullptr;
    std::vector<Ty> params;
    std::vector<llvm::Constant *> defaults;
    std::vector<std::string> names;
    bool vararg = false;
    Ty ret;
    const AstNode *body = nullptr;
    bool is_static = false;
    bool is_inline = false;
    std::vector<std::string> home;
    std::vector<LookupScope> lookup;
};

struct Declared {
    const AstNode *name = nullptr;
    const AstNode *func = nullptr;
    const AstNode *init = nullptr;
    Ty ty;
    int bits = 0;
};

class Codegen {
public:
    Codegen(llvm::LLVMContext &context, llvm::Module &module)
        : context(context), module(module), builder(context)
    {
        scopes.emplace_back();
        namespaces[""] = LookupScope{};
        module.setDataLayout(
            "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128");
    }

    int run(const AstNode *root)
    {
        if (root == NULL || root->kind != AST_TRANSLATION_UNIT) {
            error("expected a translation unit");
            return 1;
        }
        for (const AstNode *item = root->a; item != NULL; item = item->next) {
            declare_item(item);
            if (failed)
                return 1;
        }
        for (auto &group : overloads) {
            for (Func &fn : group.second) {
                if (fn.body == NULL)
                    continue;
                emit_body(fn);
                if (failed)
                    return 1;
            }
        }
        return 0;
    }

private:
    llvm::LLVMContext &context;
    llvm::Module &module;
    llvm::IRBuilder<> builder;
    llvm::Function *function = nullptr;
    llvm::BasicBlock *entry = nullptr;
    Ty ret_ty;
    bool failed = false;
    std::vector<std::map<std::string, Slot>> scopes;
    std::map<std::string, std::vector<Func>> overloads;
    std::map<std::string, Ty> aliases;
    std::map<std::string, Record> records;
    std::map<std::string, int64_t> enum_consts;
    std::set<std::string> enum_types;
    int anon_records = 0;
    std::vector<std::string> ns_stack;
    std::map<std::string, LookupScope> namespaces;
    std::vector<BlockUse> blocks;
    const std::vector<LookupScope> *active_lookup = nullptr;
    std::vector<LoopFrame> loops;
    std::vector<SwitchFrame> switches;
    SwitchFrame *collecting = nullptr;
    std::map<std::string, LabelInfo> labels;

    void error(const char *message)
    {
        if (!failed)
            std::fprintf(stderr, "%s\n", message);
        failed = true;
    }

    void error_name(const char *message, const char *name)
    {
        if (!failed)
            std::fprintf(stderr, "%s: %s\n", message, name != NULL ? name : "");
        failed = true;
    }

    void note_storage(Storage *storage, bool already, bool *flag, const char *name)
    {
        if (storage == NULL || flag == NULL) {
            error("storage class is not allowed here");
            return;
        }
        if (already) {
            error_name("duplicate storage class", name);
            return;
        }
        *flag = true;
    }

    Ty drop(Ty ty)
    {
        ty.is_const = false;
        ty.is_volatile = false;
        return ty;
    }

    Ty integer(unsigned bits, bool is_signed, int rank)
    {
        Ty ty;
        ty.ir = llvm::Type::getIntNTy(context, bits);
        ty.is_signed = is_signed;
        ty.is_bool = bits == 1;
        ty.rank = rank;
        return ty;
    }

    Ty floating(bool is_double)
    {
        Ty ty;
        ty.ir = is_double ? llvm::Type::getDoubleTy(context) : llvm::Type::getFloatTy(context);
        ty.is_float = true;
        ty.fp_rank = is_double ? 2 : 1;
        return ty;
    }

    Ty bool_ty()
    {
        return integer(1, false, 0);
    }

    Ty int_ty()
    {
        return integer(32, true, 3);
    }

    Ty size_ty()
    {
        return integer(64, false, 4);
    }

    Ty void_ty()
    {
        Ty ty;
        ty.ir = llvm::Type::getVoidTy(context);
        ty.is_void = true;
        return ty;
    }

    RValue fallback()
    {
        RValue value;
        value.ty = int_ty();
        value.v = llvm::ConstantInt::get(value.ty.ir, 0);
        return value;
    }

    bool op_is(const AstNode *node, const char *text)
    {
        return node != NULL && node->text != NULL && std::strcmp(node->text, text) == 0;
    }

    bool is_integer(const Ty &ty)
    {
        return !ty.is_void && !ty.is_float && ty.ir != NULL && ty.ir->isIntegerTy();
    }

    void reject_node(const AstNode *node)
    {
        if (node == NULL) {
            error("not in this phase");
            return;
        }
        switch (node->kind) {
        case AST_CALL:
            error("call is not allowed here");
            break;
        case AST_ARRAY:
        case AST_INDEX:
        case AST_POINTER:
        case AST_DECLARATOR:
        case AST_STRUCT:
        case AST_UNION:
        case AST_ENUM:
        case AST_ENUMERATOR:
        case AST_MEMBER:
        case AST_ARROW:
        case AST_BITFIELD:
        case AST_DESIGNATED:
            error("not allowed here");
            break;
        case AST_NULL:
            error("null is a later phase");
            break;
        case AST_NAMESPACE:
            error("namespace is not a value");
            break;
        case AST_USING:
        case AST_QUALIFIED:
            error("not allowed here");
            break;
        case AST_CLASS:
        case AST_BASE:
        case AST_ACCESS:
        case AST_CONSTRUCTOR:
        case AST_DESTRUCTOR:
        case AST_MEM_INIT:
        case AST_THIS:
            error("classes are a later phase");
            break;
        case AST_TRY:
        case AST_CATCH:
        case AST_THROW:
            error("exceptions are a later phase");
            break;
        case AST_NEW:
        case AST_DELETE:
            error("allocation is a later phase");
            break;
        case AST_PRINT:
        case AST_FORMAT:
            error("print and format are a later phase");
            break;
        case AST_TYPE_NAME:
            error("type name is not a value");
            break;
        case AST_ELLIPSIS:
            error("ellipsis is not allowed here");
            break;
        default:
            if (node->kind == AST_LITERAL && node->text != NULL
                && (node->text[0] == '"' || (node->text[0] == 'L' && node->text[1] == '"')))
                error("string is not allowed here");
            else
                error("not in this phase");
            break;
        }
    }

    void reject_declarator(const AstNode *node)
    {
        if (node == NULL)
            error("declaration has no name");
        else if (node->kind == AST_QUALIFIED)
            error("qualified names are a later phase");
        else
            error("not a declarator");
    }

    Ty pointer_to(Ty inner)
    {
        Ty ty;
        ty.ir = llvm::PointerType::get(context, 0);
        ty.is_pointer = true;
        ty.inner = std::make_shared<Ty>(inner);
        return ty;
    }

    Ty array_of(Ty elem, int length)
    {
        Ty ty;
        ty.ir = llvm::ArrayType::get(elem.ir, static_cast<uint64_t>(length));
        ty.is_array = true;
        ty.array_len = length;
        ty.inner = std::make_shared<Ty>(elem);
        return ty;
    }

    Ty decay(Ty ty)
    {
        ty = drop(ty);
        if (ty.is_array && ty.inner)
            return pointer_to(*ty.inner);
        return ty;
    }

    Ty record_type(const Record &rec)
    {
        Ty ty;
        ty.ir = rec.ir;
        ty.is_record = true;
        ty.is_union = rec.is_union;
        ty.record = rec.name;
        return ty;
    }

    Ty enum_type(const std::string &name)
    {
        Ty ty = int_ty();
        ty.is_enum = true;
        ty.record = name;
        return ty;
    }

    bool complete_type(const Ty &ty)
    {
        if (ty.is_void || ty.ir == NULL)
            return false;
        if (ty.is_pointer)
            return ty.inner != NULL;
        if (ty.is_array) {
            if (ty.array_len <= 0 || ty.inner == NULL)
                return false;
            return complete_type(*ty.inner);
        }
        if (ty.is_record) {
            auto found = records.find(ty.record);
            return found != records.end() && found->second.complete;
        }
        return true;
    }

    unsigned abi_align(llvm::Type *ty)
    {
        if (ty == NULL)
            return 1;
        return static_cast<unsigned>(module.getDataLayout().getABITypeAlign(ty).value());
    }

    unsigned record_align(const Ty &ty)
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

    void note_quals(Ty &ty, const AstNode *list)
    {
        for (const AstNode *node = list; node != NULL; node = node->next) {
            if (node->kind != AST_SPECIFIER || node->text == NULL)
                continue;
            if (std::strcmp(node->text, "const") == 0)
                ty.is_const = true;
            else if (std::strcmp(node->text, "volatile") == 0)
                ty.is_volatile = true;
        }
    }

    Ty apply_pointers(Ty base, const AstNode *node)
    {
        if (node == NULL)
            return base;
        if (node->kind != AST_POINTER) {
            error("not a pointer");
            return base;
        }
        Ty result = pointer_to(base);
        const AstNode *nested = NULL;
        const AstNode *quals = NULL;
        if (node->a != NULL && node->a->kind == AST_POINTER)
            nested = node->a;
        else if (node->a != NULL && node->a->kind == AST_SPECIFIER)
            quals = node->a;
        if (node->b != NULL && node->b->kind == AST_POINTER)
            nested = node->b;
        if (quals != NULL)
            note_quals(result, quals);
        if (nested != NULL)
            return apply_pointers(result, nested);
        return result;
    }

    bool declarator_form(const AstNode *node)
    {
        if (node == NULL)
            return false;
        switch (node->kind) {
        case AST_IDENT:
        case AST_TYPE_NAME:
        case AST_ARRAY:
        case AST_DECLARATOR:
        case AST_POINTER:
        case AST_FUNCTION_DECL:
        case AST_QUALIFIED:
        case AST_BITFIELD:
            return true;
        default:
            return false;
        }
    }

    int const_i64(const AstNode *node, int64_t &out)
    {
        Const value = eval_const(node);
        if (!value.ok || !is_integer(value.ty) || value.v == NULL) {
            if (!failed)
                error("not a constant integer");
            return 0;
        }
        out = llvm::cast<llvm::ConstantInt>(value.v)->getSExtValue();
        return 1;
    }

    Ty walk_declarator(Ty base, const AstNode *node, Declared &out)
    {
        if (failed)
            return base;
        if (node == NULL)
            return base;
        if (node->kind == AST_IDENT || node->kind == AST_TYPE_NAME) {
            out.name = node;
            return base;
        }
        if (node->kind == AST_QUALIFIED) {
            out.name = node;
            return base;
        }
        if (node->kind == AST_FUNCTION_DECL) {
            if (node->a != NULL && node->a->kind != AST_IDENT && node->a->kind != AST_QUALIFIED) {
                error("not in this phase");
                return base;
            }
            out.func = node;
            out.name = node->a;
            return base;
        }
        if (node->kind == AST_DECLARATOR)
            return walk_declarator(apply_pointers(base, node->a), node->b, out);
        if (node->kind == AST_POINTER)
            return apply_pointers(base, node);
        if (node->kind == AST_ARRAY) {
            if (node->text != NULL && std::strcmp(node->text, "[]") == 0)
                return pointer_to(base);
            if (declarator_form(node->a)) {
                if (node->b == NULL)
                    return walk_declarator(pointer_to(base), node->a, out);
                int64_t length = 0;
                if (!const_i64(node->b, length) || length <= 0 || length > (1 << 20)) {
                    if (!failed)
                        error("array size is not constant");
                    return base;
                }
                return walk_declarator(array_of(base, static_cast<int>(length)), node->a, out);
            }
            int64_t length = 0;
            if (!const_i64(node->a, length) || length <= 0 || length > (1 << 20)) {
                if (!failed)
                    error("array size is not constant");
                return base;
            }
            return array_of(base, static_cast<int>(length));
        }
        error("not a declarator");
        return base;
    }

    Declared declare_one(Ty base, const AstNode *node)
    {
        Declared out;
        out.ty = base;
        if (node != NULL && node->kind == AST_INIT) {
            out.init = node->b;
            node = node->a;
        }
        if (node != NULL && node->kind == AST_BITFIELD) {
            int64_t width = 0;
            if (!const_i64(node->b, width) || width <= 0 || width > 128) {
                if (!failed)
                    error("bit-field width is not constant");
            } else
                out.bits = static_cast<int>(width);
            node = node->a;
        }
        if (failed)
            return out;
        out.ty = walk_declarator(base, node, out);
        return out;
    }

    std::string tag_key(bool is_union, const char *text)
    {
        std::string base = text == NULL || text[0] == '\0'
            ? (is_union ? ".union.anon." : ".struct.anon.") + std::to_string(++anon_records)
            : (is_union ? "union." : "struct.") + std::string(text);
        if (ns_stack.empty())
            return base;
        return current_path() + "::" + base;
    }

    Ty make_record(const AstNode *node)
    {
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

    Ty make_enum(const AstNode *node)
    {
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

    std::optional<Field> find_field(const Ty &object, const char *name)
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

    Ty parse_specs(const AstNode *list, Where where, Storage *storage = nullptr)
    {
        int longs = 0;
        int shorts = 0;
        bool saw_signed = false;
        bool saw_unsigned = false;
        bool saw_int = false;
        bool is_const = false;
        bool is_volatile = false;
        enum Base { Missing, Void, Bool, Char, Int, Float, Double, Alias } base = Missing;
        Ty alias_ty;

        for (const AstNode *node = list; node != NULL; node = node->next) {
            if (node->kind == AST_TYPE_NAME) {
                std::optional<Ty> found = lookup_alias(node->text);
                if (!found) {
                    error_name("unknown type", node->text);
                    return int_ty();
                }
                if (base != Missing || shorts || longs || saw_signed || saw_unsigned || saw_int)
                    error("invalid type");
                base = Alias;
                alias_ty = *found;
                if (failed)
                    return int_ty();
                continue;
            }
            if (node->kind == AST_STRUCT || node->kind == AST_UNION || node->kind == AST_ENUM
                || node->kind == AST_CLASS) {
                if (node->kind == AST_CLASS) {
                    error("classes are a later phase");
                    return int_ty();
                }
                if (base != Missing || shorts || longs || saw_signed || saw_unsigned || saw_int)
                    error("invalid type");
                base = Alias;
                alias_ty = node->kind == AST_ENUM ? make_enum(node) : make_record(node);
                if (failed)
                    return int_ty();
                continue;
            }
            if (node->kind != AST_SPECIFIER || node->text == NULL) {
                reject_node(node);
                return int_ty();
            }
            const char *text = node->text;
            if (std::strcmp(text, "const") == 0) {
                if (is_const)
                    error("duplicate const");
                is_const = true;
            } else if (std::strcmp(text, "volatile") == 0) {
                if (is_volatile)
                    error("duplicate volatile");
                is_volatile = true;
            } else if (std::strcmp(text, "typedef") == 0)
                note_storage(storage, storage == NULL ? false : storage->is_typedef,
                             storage == NULL ? nullptr : &storage->is_typedef, "typedef");
            else if (std::strcmp(text, "extern") == 0)
                note_storage(storage, storage == NULL ? false : storage->is_extern,
                             storage == NULL ? nullptr : &storage->is_extern, "extern");
            else if (std::strcmp(text, "static") == 0)
                note_storage(storage, storage == NULL ? false : storage->is_static,
                             storage == NULL ? nullptr : &storage->is_static, "static");
            else if (std::strcmp(text, "inline") == 0)
                note_storage(storage, storage == NULL ? false : storage->is_inline,
                             storage == NULL ? nullptr : &storage->is_inline, "inline");
            else if (std::strcmp(text, "virtual") == 0)
                error("classes are a later phase");
            else if (std::strcmp(text, "register") == 0)
                note_storage(storage, storage == NULL ? false : storage->is_register,
                             storage == NULL ? nullptr : &storage->is_register, "register");
            else if (std::strcmp(text, "auto") == 0) {
                if (where != Where::Local)
                    error("auto is not allowed here");
            } else if (std::strcmp(text, "void") == 0) {
                if (base != Missing)
                    error("invalid type");
                base = Void;
            } else if (std::strcmp(text, "bool") == 0) {
                if (base != Missing)
                    error("invalid type");
                base = Bool;
            } else if (std::strcmp(text, "char") == 0) {
                if (base != Missing)
                    error("invalid type");
                base = Char;
            } else if (std::strcmp(text, "int") == 0) {
                if (saw_int || (base != Missing && base != Int))
                    error("invalid type");
                saw_int = true;
                if (base == Missing)
                    base = Int;
            } else if (std::strcmp(text, "short") == 0) {
                if (shorts)
                    error("invalid type");
                shorts++;
            } else if (std::strcmp(text, "long") == 0) {
                if (longs >= 2)
                    error("invalid type");
                longs++;
            } else if (std::strcmp(text, "float") == 0) {
                if (base != Missing)
                    error("invalid type");
                base = Float;
            } else if (std::strcmp(text, "double") == 0) {
                if (base != Missing)
                    error("invalid type");
                base = Double;
            } else if (std::strcmp(text, "signed") == 0) {
                if (saw_signed || saw_unsigned)
                    error("invalid type");
                saw_signed = true;
            } else if (std::strcmp(text, "unsigned") == 0) {
                if (saw_signed || saw_unsigned)
                    error("invalid type");
                saw_unsigned = true;
            } else {
                error("type is a later phase");
                return int_ty();
            }
            if (failed)
                return int_ty();
        }

        if (shorts && longs)
            error("invalid type");
        if (base == Void || base == Bool || base == Float) {
            if (shorts || longs || saw_signed || saw_unsigned || saw_int)
                error("invalid type");
        }
        if (base == Char && (shorts || longs || saw_int))
            error("invalid type");
        if (base == Double && (shorts || saw_int || saw_signed || saw_unsigned))
            error("invalid type");
        if (base == Double && longs)
            error("long double is not in this phase");
        if (base == Float && (shorts || longs))
            error("invalid type");
        if ((shorts || longs) && base != Missing && base != Int)
            error("invalid type");
        if (failed)
            return int_ty();
        if (base == Alias) {
            if (shorts || longs || saw_signed || saw_unsigned || saw_int)
                error("invalid type");
            if (failed)
                return int_ty();
            Ty ty = alias_ty;
            if (is_const)
                ty.is_const = true;
            if (is_volatile)
                ty.is_volatile = true;
            if (where == Where::Return)
                ty = drop(ty);
            return ty;
        }
        if (base == Missing && !shorts && !longs && !saw_signed && !saw_unsigned && !saw_int) {
            error("declaration has no type");
            return int_ty();
        }

        Ty ty;
        if (base == Void)
            ty = void_ty();
        else if (base == Bool)
            ty = bool_ty();
        else if (base == Float)
            ty = floating(false);
        else if (base == Double)
            ty = floating(true);
        else if (base == Char)
            ty = integer(8, !saw_unsigned, 1);
        else if (shorts)
            ty = integer(16, !saw_unsigned, 2);
        else if (longs)
            ty = integer(64, !saw_unsigned, 4);
        else
            ty = integer(32, !saw_unsigned, 3);
        ty.is_const = is_const;
        ty.is_volatile = is_volatile;
        if (where == Where::Return)
            ty = drop(ty);
        return ty;
    }

    bool is_type_form(const AstNode *node)
    {
        if (node == NULL)
            return false;
        return node->kind == AST_SPECIFIER || node->kind == AST_DECLARATOR
            || node->kind == AST_STRUCT || node->kind == AST_UNION
            || node->kind == AST_ENUM || node->kind == AST_CLASS
            || node->kind == AST_TYPE_NAME;
    }

    Ty type_from_name(const AstNode *node)
    {
        if (node != NULL && node->kind == AST_DECLARATOR) {
            Ty base = parse_specs(node->a, Where::Param);
            if (failed)
                return int_ty();
            if (node->b == NULL)
                return base;
            return declare_one(base, node->b).ty;
        }
        if (node != NULL
            && (node->kind == AST_SPECIFIER || node->kind == AST_TYPE_NAME
                || node->kind == AST_STRUCT || node->kind == AST_UNION
                || node->kind == AST_ENUM || node->kind == AST_CLASS))
            return parse_specs(node, Where::Param);
        reject_node(node);
        return int_ty();
    }

    Ty promote(Ty ty)
    {
        ty = drop(ty);
        if (ty.is_pointer || ty.is_array)
            return decay(ty);
        if (ty.is_record) {
            error("not a scalar");
            return ty;
        }
        if (ty.is_float || ty.is_void)
            return ty;
        if (!ty.is_bool && ty.rank >= 3)
            return ty;
        return int_ty();
    }

    Ty usual(Ty left, Ty right)
    {
        if (left.is_pointer || right.is_pointer || left.is_record || right.is_record
            || left.is_array || right.is_array) {
            error("invalid operands");
            return int_ty();
        }
        if (left.is_void || right.is_void) {
            error("void operand");
            return int_ty();
        }
        if (left.is_float || right.is_float) {
            if (!left.is_float)
                return drop(right);
            if (!right.is_float)
                return drop(left);
            return drop(left.fp_rank >= right.fp_rank ? left : right);
        }
        left = promote(left);
        right = promote(right);
        if (left.rank == right.rank && left.is_signed == right.is_signed)
            return left;
        if (left.is_signed == right.is_signed)
            return left.rank >= right.rank ? left : right;
        Ty uns = left.is_signed ? right : left;
        Ty sig = left.is_signed ? left : right;
        if (uns.rank >= sig.rank)
            return uns;
        return sig;
    }

    bool suffix_is(const std::string &spell, const char *suffix)
    {
        std::size_t length = spell.size();
        std::size_t mark = std::strlen(suffix);
        if (length < mark)
            return false;
        for (std::size_t i = 0; i < mark; i++) {
            if (std::tolower(static_cast<unsigned char>(spell[length - mark + i])) != suffix[i])
                return false;
        }
        return true;
    }

    bool decode_char(const char *text, unsigned &code)
    {
        if (text[0] == 'L')
            text++;
        if (text[0] != '\'')
            return false;
        text++;
        if (text[0] == '\\') {
            text++;
            if (text[0] == 'n')
                code = '\n';
            else if (text[0] == 't')
                code = '\t';
            else if (text[0] == 'r')
                code = '\r';
            else if (text[0] == 'a')
                code = '\a';
            else if (text[0] == 'b')
                code = '\b';
            else if (text[0] == 'f')
                code = '\f';
            else if (text[0] == 'v')
                code = '\v';
            else if (text[0] == '0')
                code = '\0';
            else if (text[0] == 'x' || text[0] == 'X') {
                text++;
                char *end = NULL;
                unsigned long hex = std::strtoul(text, &end, 16);
                if (end == text || hex > 0xff)
                    return false;
                code = static_cast<unsigned>(hex);
                text = end - 1;
            } else if (std::isdigit(static_cast<unsigned char>(text[0]))) {
                char *end = NULL;
                unsigned long oct = std::strtoul(text, &end, 8);
                if (end == text || oct > 0xff)
                    return false;
                code = static_cast<unsigned>(oct);
                text = end - 1;
            } else
                code = static_cast<unsigned char>(text[0]);
            text++;
        } else {
            code = static_cast<unsigned char>(text[0]);
            text++;
        }
        return text[0] == '\'' && text[1] == '\0';
    }

    bool looks_float(const char *text)
    {
        if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
            return false;
        if (std::strchr(text, '.') != NULL || std::strpbrk(text, "eE") != NULL)
            return true;
        std::size_t length = std::strlen(text);
        return length != 0 && (text[length - 1] == 'f' || text[length - 1] == 'F');
    }

    bool decode_literal(const char *text, Ty &ty, llvm::Constant *&value)
    {
        unsigned code = 0;
        if (text == NULL)
            return false;
        if (std::strcmp(text, "true") == 0) {
            ty = bool_ty();
            value = llvm::ConstantInt::get(ty.ir, 1);
            return true;
        }
        if (std::strcmp(text, "false") == 0) {
            ty = bool_ty();
            value = llvm::ConstantInt::get(ty.ir, 0);
            return true;
        }
        if (text[0] == '\'' || (text[0] == 'L' && text[1] == '\'')) {
            if (!decode_char(text, code))
                return false;
            ty = int_ty();
            value = llvm::ConstantInt::get(ty.ir, code);
            return true;
        }
        if (text[0] == '"')
            return false;
        if (looks_float(text)) {
            char *end = NULL;
            double magnitude = std::strtod(text, &end);
            if (end == text)
                return false;
            bool single = false;
            if (*end == 'f' || *end == 'F') {
                single = true;
                end++;
            } else if (*end == 'l' || *end == 'L') {
                error("long double is not in this phase");
                return false;
            }
            if (*end != '\0')
                return false;
            llvm::APFloat parsed(magnitude);
            if (single) {
                bool loses = false;
                parsed.convert(llvm::APFloat::IEEEsingle(),
                               llvm::APFloat::rmNearestTiesToEven, &loses);
            }
            ty = floating(!single);
            value = llvm::ConstantFP::get(context, parsed);
            return true;
        }

        std::string spell(text);
        enum Suf { None, U, L, UL, LL, ULL } suffix = None;
        if (suffix_is(spell, "ull") || suffix_is(spell, "llu")) {
            suffix = ULL;
            spell.resize(spell.size() - 3);
        } else if (suffix_is(spell, "ll")) {
            suffix = LL;
            spell.resize(spell.size() - 2);
        } else if (suffix_is(spell, "ul") || suffix_is(spell, "lu")) {
            suffix = UL;
            spell.resize(spell.size() - 2);
        } else if (suffix_is(spell, "u")) {
            suffix = U;
            spell.resize(spell.size() - 1);
        } else if (suffix_is(spell, "l")) {
            suffix = L;
            spell.resize(spell.size() - 1);
        }
        if (spell.empty())
            return false;
        int base = 10;
        const char *digits = spell.c_str();
        if (digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X')) {
            base = 16;
            digits += 2;
        } else if (digits[0] == '0')
            base = 8;
        if (digits[0] == '\0')
            return false;
        if (base == 8) {
            for (const char *scan = digits; *scan != '\0'; scan++) {
                if (*scan < '0' || *scan > '7')
                    return false;
            }
        }
        errno = 0;
        char *end = NULL;
        unsigned long long magnitude = std::strtoull(digits, &end, base);
        if (end == digits || *end != '\0' || errno != 0)
            return false;

        struct Fit {
            unsigned bits;
            bool is_signed;
            int rank;
        };
        Fit options[4];
        int count = 0;
        auto add = [&](unsigned bits, bool is_signed, int rank) {
            options[count++] = Fit{bits, is_signed, rank};
        };
        bool hex_or_oct = base != 10;
        if (suffix == None && !hex_or_oct) {
            add(32, true, 3);
            add(64, true, 4);
        } else if (suffix == None) {
            add(32, true, 3);
            add(32, false, 3);
            add(64, true, 4);
            add(64, false, 4);
        } else if (suffix == U) {
            add(32, false, 3);
            add(64, false, 4);
        } else if (suffix == L && !hex_or_oct)
            add(64, true, 4);
        else if (suffix == L) {
            add(64, true, 4);
            add(64, false, 4);
        } else if (suffix == UL)
            add(64, false, 4);
        else if (suffix == LL && !hex_or_oct)
            add(64, true, 4);
        else if (suffix == LL) {
            add(64, true, 4);
            add(64, false, 4);
        } else
            add(64, false, 4);

        for (int i = 0; i < count; i++) {
            bool fits;
            if (options[i].bits >= 64)
                fits = !options[i].is_signed
                    || magnitude <= static_cast<unsigned long long>(INT64_MAX);
            else if (options[i].is_signed)
                fits = magnitude <= (1ULL << (options[i].bits - 1)) - 1;
            else
                fits = magnitude <= (1ULL << options[i].bits) - 1;
            if (!fits)
                continue;
            ty = integer(options[i].bits, options[i].is_signed, options[i].rank);
            value = llvm::ConstantInt::get(ty.ir, magnitude, false);
            return true;
        }
        return false;
    }

    llvm::Constant *convert_const(llvm::Constant *value, const Ty &from, const Ty &to)
    {
        if (failed || value == NULL)
            return llvm::Constant::getNullValue(to.ir != NULL ? to.ir : llvm::Type::getInt32Ty(context));
        if (to.is_void || from.is_void) {
            if (!(to.is_void && from.is_void))
                error("cannot convert void");
            return value;
        }
        if (from.is_pointer || to.is_pointer || from.is_record || to.is_record
            || from.is_array || to.is_array) {
            if (from.ir == to.ir)
                return value;
            error("cannot convert");
            return llvm::Constant::getNullValue(to.ir);
        }
        if (from.ir == to.ir)
            return value;
        if (to.is_bool) {
            bool nonzero = false;
            if (from.is_float)
                nonzero = !llvm::cast<llvm::ConstantFP>(value)->isZero();
            else
                nonzero = !llvm::cast<llvm::ConstantInt>(value)->isZero();
            return llvm::ConstantInt::get(to.ir, nonzero ? 1 : 0);
        }
        if (to.is_float && !from.is_float) {
            auto *iconst = llvm::cast<llvm::ConstantInt>(value);
            llvm::APFloat parsed(to.fp_rank >= 2 ? llvm::APFloat::IEEEdouble()
                                                 : llvm::APFloat::IEEEsingle());
            parsed.convertFromAPInt(iconst->getValue(), from.is_signed,
                                    llvm::APFloat::rmNearestTiesToEven);
            return llvm::ConstantFP::get(context, parsed);
        }
        if (from.is_float && to.is_float) {
            llvm::APFloat parsed = llvm::cast<llvm::ConstantFP>(value)->getValueAPF();
            bool loses = false;
            parsed.convert(to.fp_rank >= 2 ? llvm::APFloat::IEEEdouble()
                                           : llvm::APFloat::IEEEsingle(),
                           llvm::APFloat::rmNearestTiesToEven, &loses);
            return llvm::ConstantFP::get(context, parsed);
        }
        if (from.is_float) {
            llvm::APFloat parsed = llvm::cast<llvm::ConstantFP>(value)->getValueAPF();
            llvm::APSInt bits(to.ir->getIntegerBitWidth(), !to.is_signed);
            bool exact = false;
            llvm::APFloat::opStatus status = parsed.convertToInteger(
                bits, llvm::APFloat::rmTowardZero, &exact);
            if (status != llvm::APFloat::opOK && status != llvm::APFloat::opInexact)
                return llvm::ConstantInt::get(to.ir, 0);
            return llvm::ConstantInt::get(context, bits);
        }
        llvm::APInt bits = llvm::cast<llvm::ConstantInt>(value)->getValue();
        unsigned width = to.ir->getIntegerBitWidth();
        if (bits.getBitWidth() > width)
            bits = bits.trunc(width);
        else if (bits.getBitWidth() < width)
            bits = from.is_signed ? bits.sext(width) : bits.zext(width);
        return llvm::ConstantInt::get(context, bits);
    }

    llvm::Value *convert(llvm::Value *value, const Ty &from, const Ty &to)
    {
        if (failed)
            return llvm::Constant::getNullValue(to.ir != NULL ? to.ir : llvm::Type::getInt32Ty(context));
        if (from.is_pointer || to.is_pointer) {
            if (from.is_pointer && to.is_pointer)
                return value;
            if (from.is_pointer && to.is_bool)
                return builder.CreateICmpNE(
                    value, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(from.ir)));
            if (from.is_pointer && is_integer(to))
                return builder.CreatePtrToInt(value, to.ir);
            if (to.is_pointer && is_integer(from))
                return builder.CreateIntToPtr(value, to.ir);
            error("cannot convert");
            return llvm::Constant::getNullValue(to.ir != NULL ? to.ir : llvm::Type::getInt32Ty(context));
        }
        if (from.is_record || to.is_record || from.is_array || to.is_array) {
            if (from.is_record && to.is_record && from.record == to.record)
                return value;
            error("cannot convert");
            return llvm::Constant::getNullValue(to.ir != NULL ? to.ir : llvm::Type::getInt32Ty(context));
        }
        if (to.is_void || from.is_void) {
            if (!(to.is_void && from.is_void))
                error("cannot convert void");
            return value;
        }
        if (from.ir == to.ir)
            return value;
        if (to.is_bool) {
            if (from.is_float)
                return builder.CreateFCmpUNE(value, llvm::ConstantFP::get(from.ir, 0.0));
            return builder.CreateICmpNE(value, llvm::ConstantInt::get(from.ir, 0));
        }
        if (to.is_float && !from.is_float) {
            if (from.is_signed)
                return builder.CreateSIToFP(value, to.ir);
            return builder.CreateUIToFP(value, to.ir);
        }
        if (from.is_float && to.is_float) {
            if (from.fp_rank < to.fp_rank)
                return builder.CreateFPExt(value, to.ir);
            if (from.fp_rank > to.fp_rank)
                return builder.CreateFPTrunc(value, to.ir);
            return value;
        }
        if (from.is_float) {
            if (to.is_signed)
                return builder.CreateFPToSI(value, to.ir);
            return builder.CreateFPToUI(value, to.ir);
        }
        unsigned src = from.ir->getIntegerBitWidth();
        unsigned dst = to.ir->getIntegerBitWidth();
        if (dst < src)
            return builder.CreateTrunc(value, to.ir);
        if (dst > src) {
            if (from.is_signed)
                return builder.CreateSExt(value, to.ir);
            return builder.CreateZExt(value, to.ir);
        }
        return value;
    }

    RValue cast_to(RValue value, Ty dest)
    {
        if (failed)
            return fallback();
        RValue out;
        out.ty = drop(dest);
        out.v = convert(value.v, value.ty, dest);
        return out;
    }

    void push_scope()
    {
        scopes.emplace_back();
        blocks.emplace_back();
    }

    void pop_scope()
    {
        if (scopes.size() > 1)
            scopes.pop_back();
        if (!blocks.empty())
            blocks.pop_back();
    }

    std::string join_ns(int count) const
    {
        std::string key;
        int limit = count;
        if (limit > static_cast<int>(ns_stack.size()))
            limit = static_cast<int>(ns_stack.size());
        for (int i = 0; i < limit; i++) {
            if (!key.empty())
                key += "::";
            key += ns_stack[static_cast<std::size_t>(i)];
        }
        return key;
    }

    std::string current_path() const
    {
        return join_ns(static_cast<int>(ns_stack.size()));
    }

    std::string qualify(const std::string &name) const
    {
        std::string path = current_path();
        if (path.empty())
            return name;
        return path + "::" + name;
    }

    bool name_known(const std::string &key) const
    {
        if (key.empty())
            return false;
        if (scopes.empty())
            return false;
        return scopes[0].count(key) != 0 || overloads.count(key) != 0 || aliases.count(key) != 0
            || enum_consts.count(key) != 0 || namespaces.count(key) != 0;
    }

    std::vector<std::string> dedup_keys(std::vector<std::string> keys) const
    {
        std::vector<std::string> out;
        for (const std::string &key : keys) {
            bool seen = false;
            for (const std::string &earlier : out) {
                if (earlier == key)
                    seen = true;
            }
            if (!seen)
                out.push_back(key);
        }
        return out;
    }

    std::vector<std::string> use_keys(const std::vector<std::pair<std::string, std::string>> &names,
                                      const std::vector<std::string> &directives, const std::string &name) const
    {
        std::vector<std::string> hits;
        for (const auto &use : names) {
            if (use.first == name && name_known(use.second))
                hits.push_back(use.second);
        }
        if (!hits.empty())
            return dedup_keys(hits);
        for (const std::string &dir : directives) {
            std::string key = dir + "::" + name;
            if (name_known(key))
                hits.push_back(key);
        }
        return dedup_keys(hits);
    }

    std::vector<std::string> scope_keys(const LookupScope &scope, const std::string &name) const
    {
        std::string key = scope.ns.empty() ? name : scope.ns + "::" + name;
        if (name_known(key))
            return {key};
        std::vector<std::string> declared = use_keys(scope.names, {}, name);
        if (!declared.empty())
            return declared;
        return use_keys({}, scope.directives, name);
    }

    std::vector<LookupScope> live_chain() const
    {
        std::vector<LookupScope> chain;
        for (int i = static_cast<int>(ns_stack.size()); i >= 0; --i) {
            std::string key = join_ns(i);
            LookupScope scope;
            auto found = namespaces.find(key);
            if (found != namespaces.end())
                scope = found->second;
            scope.ns = key;
            chain.push_back(scope);
        }
        return chain;
    }

    std::vector<std::string> visible_keys(const std::string &name) const
    {
        for (auto block = blocks.rbegin(); block != blocks.rend(); ++block) {
            std::vector<std::string> declared = use_keys(block->names, {}, name);
            if (!declared.empty())
                return declared;
            std::vector<std::string> directed = use_keys({}, block->directives, name);
            if (!directed.empty())
                return directed;
        }
        std::vector<LookupScope> live;
        const std::vector<LookupScope> *chain = active_lookup;
        if (chain == NULL) {
            live = live_chain();
            chain = &live;
        }
        for (const LookupScope &scope : *chain) {
            std::vector<std::string> hits = scope_keys(scope, name);
            if (!hits.empty())
                return hits;
        }
        return {};
    }

    void flatten_name(const AstNode *node, std::vector<const AstNode *> &parts) const
    {
        if (node == NULL)
            return;
        if (node->kind == AST_QUALIFIED) {
            flatten_name(node->a, parts);
            flatten_name(node->b, parts);
            return;
        }
        parts.push_back(node);
    }

    std::string find_ns_component(const std::string &component)
    {
        for (int i = static_cast<int>(ns_stack.size()); i >= 0; --i) {
            std::string key = join_ns(i);
            if (!key.empty())
                key += "::";
            key += component;
            if (namespaces.count(key) != 0)
                return key;
        }
        error_name("unknown namespace", component.c_str());
        return "";
    }

    std::string namespace_key(const AstNode *node)
    {
        std::vector<const AstNode *> parts;
        flatten_name(node, parts);
        if (parts.empty() || parts[0] == NULL || parts[0]->text == NULL) {
            error("unknown namespace");
            return "";
        }
        std::string path = find_ns_component(parts[0]->text);
        if (failed)
            return "";
        for (std::size_t i = 1; i < parts.size(); i++) {
            if (parts[i] == NULL || parts[i]->text == NULL) {
                error("unknown namespace");
                return "";
            }
            path += "::";
            path += parts[i]->text;
            if (namespaces.count(path) == 0) {
                error_name("unknown namespace", path.c_str());
                return "";
            }
        }
        return path;
    }

    std::string entity_key(const AstNode *node)
    {
        std::vector<const AstNode *> parts;
        flatten_name(node, parts);
        if (parts.empty() || parts.back() == NULL || parts.back()->text == NULL) {
            error("the name is missing");
            return "";
        }
        if (parts.size() == 1) {
            std::vector<std::string> keys = visible_keys(parts[0]->text);
            if (keys.size() != 1) {
                if (keys.empty())
                    error_name("unknown name", parts[0]->text);
                else
                    error_name("ambiguous name", parts[0]->text);
                return "";
            }
            return keys[0];
        }
        std::string path = find_ns_component(parts[0]->text);
        if (failed)
            return "";
        for (std::size_t i = 1; i + 1 < parts.size(); i++) {
            path += "::";
            path += parts[i]->text;
            if (namespaces.count(path) == 0) {
                error_name("unknown namespace", path.c_str());
                return "";
            }
        }
        std::string key = path + "::" + parts.back()->text;
        if (!name_known(key)) {
            error_name("unknown name", key.c_str());
            return "";
        }
        return key;
    }

    bool namespace_only_name(const char *name) const
    {
        if (name == NULL)
            return false;
        std::vector<std::string> keys = visible_keys(name);
        return keys.size() == 1 && namespace_only(keys[0]);
    }

    bool namespace_only(const std::string &key) const
    {
        return !key.empty() && namespaces.count(key) != 0 && scopes[0].count(key) == 0
            && overloads.count(key) == 0 && aliases.count(key) == 0 && enum_consts.count(key) == 0;
    }

    std::optional<int64_t> lookup_enum(const char *name)
    {
        if (name == NULL)
            return std::nullopt;
        for (const std::string &key : visible_keys(name)) {
            auto found = enum_consts.find(key);
            if (found != enum_consts.end())
                return found->second;
        }
        return std::nullopt;
    }

    std::optional<Ty> lookup_alias(const char *name)
    {
        if (name == NULL)
            return std::nullopt;
        for (const std::string &key : visible_keys(name)) {
            auto found = aliases.find(key);
            if (found != aliases.end())
                return found->second;
        }
        return std::nullopt;
    }

    void declare_item(const AstNode *item)
    {
        if (item == NULL || failed)
            return;
        if (item->kind == AST_METHOD)
            declare_function(item->a, item->b, item->c);
        else if (item->kind == AST_DECLARATION)
            emit_declaration(item, true);
        else if (item->kind == AST_NAMESPACE)
            declare_namespace(item);
        else if (item->kind == AST_USING)
            add_using(item);
        else if (item->kind == AST_CONSTRUCTOR || item->kind == AST_DESTRUCTOR)
            error("classes are a later phase");
        else
            reject_node(item);
    }

    void declare_namespace(const AstNode *node)
    {
        if (node->text == NULL) {
            error("namespace has no name");
            return;
        }
        std::string key = qualify(node->text);
        if (scopes[0].count(key) != 0 || overloads.count(key) != 0 || aliases.count(key) != 0
            || enum_consts.count(key) != 0) {
            error_name("name is already defined", node->text);
            return;
        }
        if (namespaces.count(key) == 0)
            namespaces[key] = LookupScope{};
        ns_stack.push_back(node->text);
        for (const AstNode *item = node->a; item != NULL && !failed; item = item->next)
            declare_item(item);
        ns_stack.pop_back();
    }

    void add_using(const AstNode *node)
    {
        bool directive = node->text != NULL && std::strcmp(node->text, "namespace") == 0;
        if (directive) {
            std::string target = namespace_key(node->a);
            if (failed)
                return;
            if (!blocks.empty())
                blocks.back().directives.push_back(target);
            else
                namespaces[current_path()].directives.push_back(target);
            return;
        }
        if (node->a == NULL || node->a->kind != AST_QUALIFIED) {
            error("using needs a qualified name");
            return;
        }
        std::string target = entity_key(node->a);
        if (failed)
            return;
        if (namespace_only(target)) {
            error("namespace is not a value");
            return;
        }
        std::string introduced = node->a->b != NULL && node->a->b->text != NULL ? node->a->b->text : "";
        if (introduced.empty()) {
            error("using needs a qualified name");
            return;
        }
        if (!blocks.empty())
            blocks.back().names.push_back({introduced, target});
        else
            namespaces[current_path()].names.push_back({introduced, target});
    }

    std::optional<Slot> lookup(const char *name)
    {
        if (name == NULL)
            return std::nullopt;
        for (std::size_t i = scopes.size(); i-- > 1;) {
            auto found = scopes[i].find(name);
            if (found != scopes[i].end())
                return found->second;
        }
        const Slot *hit = NULL;
        for (const std::string &key : visible_keys(name)) {
            auto found = scopes[0].find(key);
            if (found == scopes[0].end())
                continue;
            if (hit != NULL && hit->addr != found->second.addr) {
                error_name("ambiguous name", name);
                return std::nullopt;
            }
            hit = &found->second;
        }
        if (hit == NULL)
            return std::nullopt;
        return *hit;
    }

    void bind(const std::string &name, Slot slot)
    {
        if (name.empty())
            return;
        if (scopes.back().count(name) != 0 || enum_consts.count(name) != 0) {
            error_name("name is already defined", name.c_str());
            return;
        }
        scopes.back()[name] = slot;
    }

    llvm::AllocaInst *alloca_entry(llvm::Type *ty, const std::string &name)
    {
        llvm::IRBuilder<> allocas(entry, entry->begin());
        return allocas.CreateAlloca(ty, nullptr, name);
    }

    llvm::Value *load_slot(const Slot &slot)
    {
        if (slot.ty.is_array) {
            error("not a scalar");
            return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0);
        }
        if (slot.bits > 0) {
            llvm::Type *narrow_ty = llvm::Type::getIntNTy(context, static_cast<unsigned>(slot.bits));
            llvm::Value *raw = builder.CreateLoad(narrow_ty, slot.addr, slot.ty.is_volatile);
            Ty narrow = slot.ty;
            narrow.ir = narrow_ty;
            narrow.is_bool = false;
            return convert(raw, narrow, slot.ty);
        }
        return builder.CreateLoad(slot.ty.ir, slot.addr, slot.ty.is_volatile);
    }

    void store_slot(const Slot &slot, llvm::Value *value, bool initializing)
    {
        if (slot.ty.is_const && !initializing) {
            error("assignment to const");
            return;
        }
        if (slot.bits > 0) {
            Ty narrow = slot.ty;
            narrow.ir = llvm::Type::getIntNTy(context, static_cast<unsigned>(slot.bits));
            narrow.is_bool = false;
            value = convert(value, slot.ty, narrow);
        }
        builder.CreateStore(value, slot.addr, slot.ty.is_volatile);
    }

    RValue read_slot(const Slot &slot)
    {
        if (slot.ty.is_array && slot.ty.inner != NULL && slot.bits == 0) {
            llvm::Value *zero = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0);
            RValue out;
            out.ty = pointer_to(*slot.ty.inner);
            out.v = builder.CreateInBoundsGEP(slot.ty.ir, slot.addr, {zero, zero});
            return out;
        }
        RValue out;
        out.v = load_slot(slot);
        out.ty = drop(slot.ty);
        return out;
    }

    Slot field_addr(llvm::Value *base, const Ty &object, const Field &field)
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

    Ty object_type(const AstNode *node)
    {
        if (failed || node == NULL) {
            error("assignment target is not a scalar");
            return int_ty();
        }
        if (node->kind == AST_IDENT || node->kind == AST_QUALIFIED) {
            if (node->kind == AST_QUALIFIED) {
                std::string key = entity_key(node);
                if (failed)
                    return int_ty();
                auto found = scopes[0].find(key);
                if (found == scopes[0].end()) {
                    error_name("unknown name", key.c_str());
                    return int_ty();
                }
                return found->second.ty;
            }
            std::optional<Slot> slot = lookup(node->text);
            if (!slot) {
                if (lookup_enum(node->text))
                    error("assignment target is not a scalar");
                else if (node->text != NULL && namespace_only_name(node->text))
                    error("namespace is not a value");
                else
                    error_name("unknown name", node->text);
                return int_ty();
            }
            return slot->ty;
        }
        if (node->kind == AST_MEMBER || node->kind == AST_ARROW) {
            Ty base = node->kind == AST_ARROW ? decay(type_of(node->a)) : object_type(node->a);
            if (failed)
                return int_ty();
            if (node->kind == AST_ARROW) {
                if (!base.is_pointer || base.inner == NULL) {
                    error("not a pointer");
                    return int_ty();
                }
                base = *base.inner;
            }
            std::optional<Field> field = find_field(base, node->text);
            if (!field)
                return int_ty();
            return field->ty;
        }
        if (node->kind == AST_INDEX) {
            Ty base = decay(type_of(node->a));
            type_of(node->b);
            if (failed)
                return int_ty();
            if (!base.is_pointer || base.inner == NULL) {
                error("not an array or pointer");
                return int_ty();
            }
            return *base.inner;
        }
        if (node->kind == AST_UNARY && op_is(node, "*")) {
            Ty base = decay(type_of(node->a));
            if (!base.is_pointer || base.inner == NULL) {
                error("not a pointer");
                return int_ty();
            }
            return *base.inner;
        }
        error("assignment target is not a scalar");
        return int_ty();
    }

    std::optional<Slot> index_slot(const AstNode *node)
    {
        RValue index = emit_rvalue(node->b);
        if (failed)
            return std::nullopt;
        if (!is_integer(index.ty)) {
            error("subscript is not an integer");
            return std::nullopt;
        }
        llvm::Value *idx = convert(index.v, index.ty, integer(64, true, 4));
        RValue base = emit_rvalue(node->a);
        if (failed)
            return std::nullopt;
        if (!base.ty.is_pointer || base.ty.inner == NULL) {
            error("not an array or pointer");
            return std::nullopt;
        }
        Slot slot;
        slot.ty = *base.ty.inner;
        slot.addr = builder.CreateInBoundsGEP(base.ty.inner->ir, base.v, {idx});
        return slot;
    }

    bool terminated()
    {
        llvm::BasicBlock *block = builder.GetInsertBlock();
        return block == NULL || block->getTerminator() != NULL;
    }

    void ensure_insert()
    {
        if (failed)
            return;
        if (!terminated())
            return;
        llvm::BasicBlock *dead = llvm::BasicBlock::Create(context, "dead", function);
        builder.SetInsertPoint(dead);
    }

    llvm::BasicBlock *new_block(const char *name)
    {
        return llvm::BasicBlock::Create(context, name, function);
    }

    std::optional<Slot> lvalue(const AstNode *node)
    {
        if (failed || node == NULL) {
            error("assignment target is not a scalar");
            return std::nullopt;
        }
        if (node->kind == AST_IDENT) {
            if (lookup_enum(node->text) && !lookup(node->text)) {
                error("assignment target is not a scalar");
                return std::nullopt;
            }
            if (node->text != NULL && namespace_only_name(node->text)) {
                error("namespace is not a value");
                return std::nullopt;
            }
            std::optional<Slot> slot = lookup(node->text);
            if (!slot)
                error_name("unknown name", node->text);
            return slot;
        }
        if (node->kind == AST_QUALIFIED) {
            std::string key = entity_key(node);
            if (failed)
                return std::nullopt;
            if (namespace_only(key)) {
                error("namespace is not a value");
                return std::nullopt;
            }
            auto found = scopes[0].find(key);
            if (found == scopes[0].end()) {
                error_name("unknown name", key.c_str());
                return std::nullopt;
            }
            return found->second;
        }
        if (node->kind == AST_MEMBER) {
            std::optional<Slot> base = lvalue(node->a);
            if (!base)
                return std::nullopt;
            std::optional<Field> field = find_field(base->ty, node->text);
            if (!field)
                return std::nullopt;
            return field_addr(base->addr, base->ty, *field);
        }
        if (node->kind == AST_ARROW) {
            RValue ptr = emit_rvalue(node->a);
            if (failed)
                return std::nullopt;
            if (!ptr.ty.is_pointer || ptr.ty.inner == NULL) {
                error("not a pointer");
                return std::nullopt;
            }
            std::optional<Field> field = find_field(*ptr.ty.inner, node->text);
            if (!field)
                return std::nullopt;
            return field_addr(ptr.v, *ptr.ty.inner, *field);
        }
        if (node->kind == AST_INDEX)
            return index_slot(node);
        if (node->kind == AST_UNARY && op_is(node, "*")) {
            RValue ptr = emit_rvalue(node->a);
            if (failed)
                return std::nullopt;
            if (!ptr.ty.is_pointer || ptr.ty.inner == NULL) {
                error("not a pointer");
                return std::nullopt;
            }
            Slot slot;
            slot.addr = ptr.v;
            slot.ty = *ptr.ty.inner;
            return slot;
        }
        error("assignment target is not a scalar");
        return std::nullopt;
    }

    Ty type_of(const AstNode *node)
    {
        if (failed || node == NULL)
            return int_ty();
        if (node->kind == AST_LITERAL) {
            if (is_string(node))
                return string_ty();
            Ty ty;
            llvm::Constant *value = nullptr;
            if (!decode_literal(node->text, ty, value)) {
                if (!failed)
                    error("bad literal");
                return int_ty();
            }
            return ty;
        }
        if (node->kind == AST_IDENT) {
            std::optional<Slot> slot = lookup(node->text);
            if (!slot) {
                if (lookup_enum(node->text))
                    return int_ty();
                if (node->text != NULL && namespace_only_name(node->text))
                    error("namespace is not a value");
                else
                    error_name("unknown name", node->text);
                return int_ty();
            }
            return drop(slot->ty);
        }
        if (node->kind == AST_QUALIFIED) {
            std::string key = entity_key(node);
            if (failed)
                return int_ty();
            if (namespace_only(key)) {
                error("namespace is not a value");
                return int_ty();
            }
            auto global = scopes[0].find(key);
            if (global != scopes[0].end())
                return drop(global->second.ty);
            if (enum_consts.count(key) != 0)
                return int_ty();
            error_name("unknown name", key.c_str());
            return int_ty();
        }
        if (node->kind == AST_MEMBER || node->kind == AST_ARROW || node->kind == AST_INDEX)
            return drop(object_type(node));
        if (node->kind == AST_SIZEOF)
            return size_ty();
        if (node->kind == AST_CAST) {
            Ty ty = type_from_name(node->a);
            return drop(ty);
        }
        if (node->kind == AST_UNARY)
            return type_unary(node);
        if (node->kind == AST_BINARY)
            return type_binary(node);
        if (node->kind == AST_ASSIGN)
            return drop(object_type(node->a));
        if (node->kind == AST_CONDITIONAL) {
            Ty left = type_of(node->b);
            Ty right = type_of(node->c);
            if (left.is_void || right.is_void) {
                error("conditional operand is void");
                return int_ty();
            }
            return usual(left, right);
        }
        if (node->kind == AST_CALL)
            return type_call(node);
        reject_node(node);
        return int_ty();
    }

    Ty type_unary(const AstNode *node)
    {
        if (op_is(node, "&")) {
            Ty object = object_type(node->a);
            if (failed)
                return int_ty();
            return pointer_to(object);
        }
        if (op_is(node, "*"))
            return drop(object_type(node));
        if (op_is(node, "!"))
            return bool_ty();
        if (op_is(node, "++") || op_is(node, "--") || op_is(node, "post++")
            || op_is(node, "post--"))
            return drop(object_type(node->a));
        Ty inner = type_of(node->a);
        if (op_is(node, "~") && inner.is_float)
            error("complement of a float");
        return promote(inner);
    }

    Ty type_binary(const AstNode *node)
    {
        if (op_is(node, "&&") || op_is(node, "||"))
            return bool_ty();
        if (op_is(node, ",")) {
            type_of(node->a);
            return type_of(node->b);
        }
        Ty left = decay(type_of(node->a));
        Ty right = decay(type_of(node->b));
        if (op_is(node, "<") || op_is(node, ">") || op_is(node, "<=") || op_is(node, ">=")
            || op_is(node, "==") || op_is(node, "!="))
            return bool_ty();
        if (left.is_pointer || right.is_pointer) {
            bool add = op_is(node, "+");
            bool sub = op_is(node, "-");
            if (add && left.is_pointer && is_integer(right))
                return left;
            if (add && right.is_pointer && is_integer(left))
                return right;
            if (sub && left.is_pointer && is_integer(right))
                return left;
            if (sub && left.is_pointer && right.is_pointer)
                return integer(64, true, 4);
            error("invalid operands");
            return int_ty();
        }
        if (op_is(node, "<<") || op_is(node, ">>")) {
            if (left.is_float || right.is_float)
                error("shift of a float");
            return promote(left);
        }
        if ((op_is(node, "&") || op_is(node, "|") || op_is(node, "^"))
            && (left.is_float || right.is_float))
            error("bitwise operator on a float");
        return usual(left, right);
    }

    Const eval_const(const AstNode *node)
    {
        Const none;
        if (failed || node == NULL)
            return none;
        if (node->kind == AST_LITERAL) {
            Const out;
            if (!decode_literal(node->text, out.ty, out.v)) {
                if (!failed)
                    error("bad literal");
                return none;
            }
            out.ok = true;
            return out;
        }
        if (node->kind == AST_SIZEOF)
            return eval_sizeof(node->a);
        if (node->kind == AST_CAST) {
            Const inner = eval_const(node->b);
            if (!inner.ok)
                return none;
            Ty dest = type_from_name(node->a);
            if (failed)
                return none;
            if (dest.is_void) {
                Const out;
                out.ok = true;
                out.ty = void_ty();
                out.v = nullptr;
                return out;
            }
            Const out;
            out.ok = true;
            out.ty = drop(dest);
            out.v = convert_const(inner.v, inner.ty, dest);
            return out;
        }
        if (node->kind == AST_UNARY)
            return eval_unary(node);
        if (node->kind == AST_BINARY)
            return eval_binary(node);
        if (node->kind == AST_CONDITIONAL)
            return eval_conditional(node);
        if (node->kind == AST_IDENT) {
            std::optional<int64_t> enumerator = lookup_enum(node->text);
            if (enumerator && !lookup(node->text)) {
                Const out;
                out.ok = true;
                out.ty = int_ty();
                out.v = llvm::ConstantInt::get(context, llvm::APInt(32, static_cast<uint64_t>(*enumerator), true));
                return out;
            }
            if (!lookup(node->text))
                error_name("unknown name", node->text);
            return none;
        }
        if (is_string(node)) {
            error("string is not a scalar constant");
            return none;
        }
        if (node->kind == AST_CALL) {
            error("call is not constant");
            return none;
        }
        if (node->kind == AST_ASSIGN)
            return none;
        reject_node(node);
        return none;
    }

    Const eval_sizeof(const AstNode *operand)
    {
        Const none;
        Ty ty = is_type_form(operand) ? type_from_name(operand) : type_of(operand);
        if (failed || ty.is_void || ty.ir == NULL) {
            if (!failed)
                error("sizeof void");
            return none;
        }
        uint64_t bytes = module.getDataLayout().getTypeAllocSize(ty.ir).getFixedValue();
        Const out;
        out.ok = true;
        out.ty = size_ty();
        out.v = llvm::ConstantInt::get(out.ty.ir, bytes);
        return out;
    }

    Const with_const(llvm::Constant *value, Ty ty)
    {
        Const out;
        out.ok = true;
        out.v = value;
        out.ty = ty;
        return out;
    }

    Const eval_unary(const AstNode *node)
    {
        Const none;
        if (op_is(node, "&") || op_is(node, "*") || op_is(node, "++") || op_is(node, "--")
            || op_is(node, "post++") || op_is(node, "post--"))
            return none;
        Const inner = eval_const(node->a);
        if (!inner.ok || inner.ty.is_void)
            return none;
        if (op_is(node, "!")) {
            bool nonzero = const_true(inner);
            return with_const(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), nonzero ? 0 : 1),
                              bool_ty());
        }
        if (op_is(node, "~") && inner.ty.is_float) {
            error("complement of a float");
            return none;
        }
        Ty dest = promote(inner.ty);
        llvm::Constant *value = convert_const(inner.v, inner.ty, dest);
        if (op_is(node, "+"))
            return with_const(value, dest);
        if (dest.is_float) {
            llvm::APFloat parsed = llvm::cast<llvm::ConstantFP>(value)->getValueAPF();
            parsed.changeSign();
            return with_const(llvm::ConstantFP::get(context, parsed), dest);
        }
        llvm::APInt bits = llvm::cast<llvm::ConstantInt>(value)->getValue();
        if (op_is(node, "-"))
            bits = -bits;
        else
            bits = ~bits;
        return with_const(llvm::ConstantInt::get(context, bits), dest);
    }

    bool const_true(const Const &value)
    {
        if (value.ty.is_float)
            return !llvm::cast<llvm::ConstantFP>(value.v)->isZero();
        return !llvm::cast<llvm::ConstantInt>(value.v)->isZero();
    }

    Const eval_binary(const AstNode *node)
    {
        Const none;
        Const left = eval_const(node->a);
        Const right = eval_const(node->b);
        if (!left.ok || !right.ok)
            return none;
        if (op_is(node, ","))
            return right;
        if (op_is(node, "&&") || op_is(node, "||")) {
            bool result = op_is(node, "&&") ? (const_true(left) && const_true(right))
                                            : (const_true(left) || const_true(right));
            return with_const(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), result ? 1 : 0),
                              bool_ty());
        }
        if (left.ty.is_void || right.ty.is_void)
            return none;
        const char *op = node->text != NULL ? node->text : "";
        bool compare = std::strcmp(op, "<") == 0 || std::strcmp(op, ">") == 0
            || std::strcmp(op, "<=") == 0 || std::strcmp(op, ">=") == 0
            || std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0;
        bool shift = std::strcmp(op, "<<") == 0 || std::strcmp(op, ">>") == 0;
        if (shift) {
            if (left.ty.is_float || right.ty.is_float) {
                error("shift of a float");
                return none;
            }
            Ty dest = promote(left.ty);
            llvm::Constant *lhs = convert_const(left.v, left.ty, dest);
            llvm::Constant *rhs = convert_const(right.v, right.ty, dest);
            llvm::APInt a = llvm::cast<llvm::ConstantInt>(lhs)->getValue();
            llvm::APInt b = llvm::cast<llvm::ConstantInt>(rhs)->getValue();
            if (b.uge(a.getBitWidth())) {
                error("shift is too wide");
                return none;
            }
            llvm::APInt folded = std::strcmp(op, "<<") == 0 ? a.shl(b)
                : (dest.is_signed ? a.ashr(b) : a.lshr(b));
            return with_const(llvm::ConstantInt::get(context, folded), dest);
        }
        if ((std::strcmp(op, "&") == 0 || std::strcmp(op, "|") == 0 || std::strcmp(op, "^") == 0)
            && (left.ty.is_float || right.ty.is_float)) {
            error("bitwise operator on a float");
            return none;
        }
        Ty dest = usual(left.ty, right.ty);
        llvm::Constant *lhs = convert_const(left.v, left.ty, dest);
        llvm::Constant *rhs = convert_const(right.v, right.ty, dest);
        if (dest.is_float)
            return fold_float(op, lhs, rhs, dest, compare);
        return fold_int(op, lhs, rhs, dest, compare);
    }

    Const fold_int(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare)
    {
        Const none;
        llvm::APInt a = llvm::cast<llvm::ConstantInt>(lhs)->getValue();
        llvm::APInt b = llvm::cast<llvm::ConstantInt>(rhs)->getValue();
        if (compare) {
            bool result = false;
            if (std::strcmp(op, "==") == 0)
                result = a.eq(b);
            else if (std::strcmp(op, "!=") == 0)
                result = a.ne(b);
            else if (dest.is_signed) {
                if (std::strcmp(op, "<") == 0)
                    result = a.slt(b);
                else if (std::strcmp(op, ">") == 0)
                    result = a.sgt(b);
                else if (std::strcmp(op, "<=") == 0)
                    result = a.sle(b);
                else
                    result = a.sge(b);
            } else if (std::strcmp(op, "<") == 0)
                result = a.ult(b);
            else if (std::strcmp(op, ">") == 0)
                result = a.ugt(b);
            else if (std::strcmp(op, "<=") == 0)
                result = a.ule(b);
            else
                result = a.uge(b);
            return with_const(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), result ? 1 : 0),
                              bool_ty());
        }
        if ((std::strcmp(op, "/") == 0 || std::strcmp(op, "%") == 0) && b.isZero()) {
            error("division by zero");
            return none;
        }
        llvm::APInt folded(a.getBitWidth(), 0);
        if (std::strcmp(op, "+") == 0)
            folded = a + b;
        else if (std::strcmp(op, "-") == 0)
            folded = a - b;
        else if (std::strcmp(op, "*") == 0)
            folded = a * b;
        else if (std::strcmp(op, "/") == 0)
            folded = dest.is_signed ? a.sdiv(b) : a.udiv(b);
        else if (std::strcmp(op, "%") == 0)
            folded = dest.is_signed ? a.srem(b) : a.urem(b);
        else if (std::strcmp(op, "&") == 0)
            folded = a & b;
        else if (std::strcmp(op, "|") == 0)
            folded = a | b;
        else if (std::strcmp(op, "^") == 0)
            folded = a ^ b;
        else {
            error("not in this phase");
            return none;
        }
        return with_const(llvm::ConstantInt::get(context, folded), dest);
    }

    Const fold_float(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare)
    {
        Const none;
        llvm::APFloat a = llvm::cast<llvm::ConstantFP>(lhs)->getValueAPF();
        llvm::APFloat b = llvm::cast<llvm::ConstantFP>(rhs)->getValueAPF();
        if (compare) {
            llvm::APFloat::cmpResult relation = a.compare(b);
            bool result = false;
            if (std::strcmp(op, "==") == 0)
                result = relation == llvm::APFloat::cmpEqual;
            else if (std::strcmp(op, "!=") == 0)
                result = relation != llvm::APFloat::cmpEqual;
            else if (std::strcmp(op, "<") == 0)
                result = relation == llvm::APFloat::cmpLessThan;
            else if (std::strcmp(op, ">") == 0)
                result = relation == llvm::APFloat::cmpGreaterThan;
            else if (std::strcmp(op, "<=") == 0)
                result = relation == llvm::APFloat::cmpLessThan
                    || relation == llvm::APFloat::cmpEqual;
            else
                result = relation == llvm::APFloat::cmpGreaterThan
                    || relation == llvm::APFloat::cmpEqual;
            return with_const(llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), result ? 1 : 0),
                              bool_ty());
        }
        if ((std::strcmp(op, "/") == 0 || std::strcmp(op, "%") == 0) && b.isZero()) {
            error("division by zero");
            return none;
        }
        llvm::APFloat::opStatus status = llvm::APFloat::opOK;
        if (std::strcmp(op, "+") == 0)
            status = a.add(b, llvm::APFloat::rmNearestTiesToEven);
        else if (std::strcmp(op, "-") == 0)
            status = a.subtract(b, llvm::APFloat::rmNearestTiesToEven);
        else if (std::strcmp(op, "*") == 0)
            status = a.multiply(b, llvm::APFloat::rmNearestTiesToEven);
        else if (std::strcmp(op, "/") == 0)
            status = a.divide(b, llvm::APFloat::rmNearestTiesToEven);
        else if (std::strcmp(op, "%") == 0)
            status = a.remainder(b);
        else {
            error("not in this phase");
            return none;
        }
        (void) status;
        return with_const(llvm::ConstantFP::get(context, a), dest);
    }

    Const eval_conditional(const AstNode *node)
    {
        Const none;
        Const cond = eval_const(node->a);
        Const left = eval_const(node->b);
        Const right = eval_const(node->c);
        if (!cond.ok || !left.ok || !right.ok)
            return none;
        if (left.ty.is_void || right.ty.is_void) {
            error("conditional operand is void");
            return none;
        }
        Ty dest = usual(left.ty, right.ty);
        llvm::Constant *taken = const_true(cond) ? convert_const(left.v, left.ty, dest)
                                                 : convert_const(right.v, right.ty, dest);
        return with_const(taken, dest);
    }

    RValue emit_rvalue(const AstNode *node)
    {
        if (failed || node == NULL)
            return fallback();
        if (node->kind == AST_LITERAL) {
            if (is_string(node))
                return emit_string(node->text);
            RValue out;
            llvm::Constant *constant = nullptr;
            if (!decode_literal(node->text, out.ty, constant)) {
                if (!failed)
                    error("bad literal");
                return fallback();
            }
            out.v = constant;
            return out;
        }
        if (node->kind == AST_IDENT) {
            std::optional<Slot> slot = lookup(node->text);
            if (!slot) {
                std::optional<int64_t> enumerator = lookup_enum(node->text);
                if (enumerator) {
                    RValue out;
                    out.ty = int_ty();
                    out.v = llvm::ConstantInt::get(context, llvm::APInt(32, static_cast<uint64_t>(*enumerator), true));
                    return out;
                }
                if (node->text != NULL && namespace_only_name(node->text))
                    error("namespace is not a value");
                else
                    error_name("unknown name", node->text);
                return fallback();
            }
            return read_slot(*slot);
        }
        if (node->kind == AST_QUALIFIED) {
            std::string key = entity_key(node);
            if (failed)
                return fallback();
            if (namespace_only(key)) {
                error("namespace is not a value");
                return fallback();
            }
            auto global = scopes[0].find(key);
            if (global != scopes[0].end())
                return read_slot(global->second);
            auto enumerator = enum_consts.find(key);
            if (enumerator != enum_consts.end()) {
                RValue out;
                out.ty = int_ty();
                out.v = llvm::ConstantInt::get(
                    context, llvm::APInt(32, static_cast<uint64_t>(enumerator->second), true));
                return out;
            }
            error_name("unknown name", key.c_str());
            return fallback();
        }
        if (node->kind == AST_MEMBER || node->kind == AST_ARROW || node->kind == AST_INDEX) {
            std::optional<Slot> slot = lvalue(node);
            if (!slot)
                return fallback();
            return read_slot(*slot);
        }
        if (node->kind == AST_SIZEOF) {
            Const size = eval_sizeof(node->a);
            if (!size.ok) {
                if (!failed)
                    error("sizeof is not constant");
                return fallback();
            }
            return RValue{size.v, size.ty};
        }
        if (node->kind == AST_CAST)
            return emit_cast(node);
        if (node->kind == AST_UNARY)
            return emit_unary(node);
        if (node->kind == AST_BINARY)
            return emit_binary(node);
        if (node->kind == AST_ASSIGN)
            return emit_assign(node);
        if (node->kind == AST_CONDITIONAL)
            return emit_conditional(node);
        if (node->kind == AST_CALL)
            return emit_call(node);
        reject_node(node);
        return fallback();
    }

    RValue emit_cast(const AstNode *node)
    {
        Ty dest = type_from_name(node->a);
        if (failed)
            return fallback();
        RValue value = emit_rvalue(node->b);
        if (failed)
            return fallback();
        if (dest.is_void) {
            RValue out;
            out.ty = void_ty();
            return out;
        }
        return cast_to(value, dest);
    }

    RValue emit_unary(const AstNode *node)
    {
        if (op_is(node, "&")) {
            std::optional<Slot> slot = lvalue(node->a);
            if (!slot)
                return fallback();
            if (slot->bits > 0) {
                error("address of a bit-field");
                return fallback();
            }
            RValue out;
            out.ty = pointer_to(slot->ty);
            out.v = slot->addr;
            return out;
        }
        if (op_is(node, "*")) {
            std::optional<Slot> slot = lvalue(node);
            if (!slot)
                return fallback();
            return read_slot(*slot);
        }
        if (op_is(node, "++") || op_is(node, "--") || op_is(node, "post++")
            || op_is(node, "post--"))
            return emit_inc(node);
        if (op_is(node, "!")) {
            llvm::Value *value = emit_bool(node->a);
            if (failed)
                return fallback();
            RValue out;
            out.v = builder.CreateNot(value);
            out.ty = bool_ty();
            return out;
        }
        RValue inner = emit_rvalue(node->a);
        if (failed)
            return fallback();
        if (inner.ty.is_void) {
            error("void operand");
            return fallback();
        }
        if (op_is(node, "~") && inner.ty.is_float) {
            error("complement of a float");
            return fallback();
        }
        Ty dest = promote(inner.ty);
        inner = cast_to(inner, dest);
        RValue out;
        out.ty = dest;
        if (op_is(node, "+"))
            out.v = inner.v;
        else if (dest.is_float)
            out.v = builder.CreateFNeg(inner.v);
        else if (op_is(node, "-"))
            out.v = builder.CreateNeg(inner.v);
        else
            out.v = builder.CreateNot(inner.v);
        return out;
    }

    RValue emit_inc(const AstNode *node)
    {
        std::optional<Slot> slot = lvalue(node->a);
        if (!slot || failed)
            return fallback();
        if (slot->ty.is_const) {
            error("assignment to const");
            return fallback();
        }
        bool postfix = op_is(node, "post++") || op_is(node, "post--");
        bool decrement = op_is(node, "--") || op_is(node, "post--");
        if (slot->ty.is_array) {
            error("not a scalar");
            return fallback();
        }
        if (slot->ty.is_pointer && slot->ty.inner != NULL) {
            llvm::Value *old = load_slot(*slot);
            llvm::Value *step = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), decrement ? -1 : 1);
            llvm::Value *next = builder.CreateInBoundsGEP(slot->ty.inner->ir, old, {step});
            store_slot(*slot, next, false);
            RValue out;
            out.ty = drop(slot->ty);
            out.v = postfix ? old : next;
            return out;
        }
        llvm::Value *old = load_slot(*slot);
        Ty object = drop(slot->ty);
        Ty widened = promote(object);
        llvm::Value *current = convert(old, object, widened);
        llvm::Value *one = widened.is_float ? static_cast<llvm::Value *>(llvm::ConstantFP::get(widened.ir, 1.0))
                                            : llvm::ConstantInt::get(widened.ir, 1);
        llvm::Value *next = widened.is_float
            ? (decrement ? builder.CreateFSub(current, one) : builder.CreateFAdd(current, one))
            : (decrement ? builder.CreateSub(current, one) : builder.CreateAdd(current, one));
        llvm::Value *stored = convert(next, widened, object);
        store_slot(*slot, stored, false);
        RValue out;
        out.ty = object;
        out.v = postfix ? old : stored;
        return out;
    }

    llvm::Value *emit_bool(const AstNode *node)
    {
        RValue value = emit_rvalue(node);
        if (failed)
            return llvm::ConstantInt::getFalse(context);
        if (value.ty.is_pointer)
            return builder.CreateICmpNE(
                value.v, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(value.ty.ir)));
        if (value.ty.is_record || value.ty.is_array) {
            error("not a scalar");
            return llvm::ConstantInt::getFalse(context);
        }
        if (value.ty.is_void) {
            error("void is not a condition");
            return llvm::ConstantInt::getFalse(context);
        }
        if (value.ty.is_bool)
            return value.v;
        if (value.ty.is_float)
            return builder.CreateFCmpUNE(value.v, llvm::ConstantFP::get(value.ty.ir, 0.0));
        return builder.CreateICmpNE(value.v, llvm::ConstantInt::get(value.ty.ir, 0));
    }

    RValue emit_and(const AstNode *left, const AstNode *right)
    {
        llvm::Value *lhs = emit_bool(left);
        if (failed)
            return fallback();
        llvm::BasicBlock *lhs_block = builder.GetInsertBlock();
        llvm::BasicBlock *rhs_block = new_block("and.rhs");
        llvm::BasicBlock *end_block = new_block("and.end");
        builder.CreateCondBr(lhs, rhs_block, end_block);
        builder.SetInsertPoint(rhs_block);
        llvm::Value *rhs = emit_bool(right);
        if (failed)
            return fallback();
        llvm::BasicBlock *rhs_end = builder.GetInsertBlock();
        builder.CreateBr(end_block);
        builder.SetInsertPoint(end_block);
        llvm::PHINode *phi = builder.CreatePHI(llvm::Type::getInt1Ty(context), 2);
        phi->addIncoming(llvm::ConstantInt::getFalse(context), lhs_block);
        phi->addIncoming(rhs, rhs_end);
        return RValue{phi, bool_ty()};
    }

    RValue emit_or(const AstNode *left, const AstNode *right)
    {
        llvm::Value *lhs = emit_bool(left);
        if (failed)
            return fallback();
        llvm::BasicBlock *lhs_block = builder.GetInsertBlock();
        llvm::BasicBlock *rhs_block = new_block("or.rhs");
        llvm::BasicBlock *end_block = new_block("or.end");
        builder.CreateCondBr(lhs, end_block, rhs_block);
        builder.SetInsertPoint(rhs_block);
        llvm::Value *rhs = emit_bool(right);
        if (failed)
            return fallback();
        llvm::BasicBlock *rhs_end = builder.GetInsertBlock();
        builder.CreateBr(end_block);
        builder.SetInsertPoint(end_block);
        llvm::PHINode *phi = builder.CreatePHI(llvm::Type::getInt1Ty(context), 2);
        phi->addIncoming(llvm::ConstantInt::getTrue(context), lhs_block);
        phi->addIncoming(rhs, rhs_end);
        return RValue{phi, bool_ty()};
    }

    RValue emit_binary(const AstNode *node)
    {
        if (op_is(node, "&&"))
            return emit_and(node->a, node->b);
        if (op_is(node, "||"))
            return emit_or(node->a, node->b);
        if (op_is(node, ",")) {
            emit_rvalue(node->a);
            if (failed)
                return fallback();
            return emit_rvalue(node->b);
        }
        RValue left = emit_rvalue(node->a);
        RValue right = emit_rvalue(node->b);
        if (failed)
            return fallback();
        return apply_op(node->text, left, right);
    }

    RValue pointer_binary(const char *op, RValue left, RValue right, bool compare)
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

    RValue apply_op(const char *op, RValue left, RValue right)
    {
        if (op == NULL) {
            error("not in this phase");
            return fallback();
        }
        bool compare = std::strcmp(op, "<") == 0 || std::strcmp(op, ">") == 0
            || std::strcmp(op, "<=") == 0 || std::strcmp(op, ">=") == 0
            || std::strcmp(op, "==") == 0 || std::strcmp(op, "!=") == 0;
        left.ty = decay(left.ty);
        right.ty = decay(right.ty);
        if (left.ty.is_pointer || right.ty.is_pointer)
            return pointer_binary(op, left, right, compare);
        bool shift = std::strcmp(op, "<<") == 0 || std::strcmp(op, ">>") == 0;
        if (shift) {
            if (left.ty.is_float || right.ty.is_float || left.ty.is_void || right.ty.is_void) {
                error("shift of a float");
                return fallback();
            }
            Ty dest = promote(left.ty);
            left = cast_to(left, dest);
            right = cast_to(right, dest);
            RValue out;
            out.ty = dest;
            if (std::strcmp(op, "<<") == 0)
                out.v = builder.CreateShl(left.v, right.v);
            else if (dest.is_signed)
                out.v = builder.CreateAShr(left.v, right.v);
            else
                out.v = builder.CreateLShr(left.v, right.v);
            return out;
        }
        if ((std::strcmp(op, "&") == 0 || std::strcmp(op, "|") == 0 || std::strcmp(op, "^") == 0)
            && (left.ty.is_float || right.ty.is_float)) {
            error("bitwise operator on a float");
            return fallback();
        }
        Ty dest = usual(left.ty, right.ty);
        if (failed)
            return fallback();
        left = cast_to(left, dest);
        right = cast_to(right, dest);
        RValue out;
        if (compare) {
            out.ty = bool_ty();
            if (dest.is_float) {
                llvm::CmpInst::Predicate pred = llvm::CmpInst::FCMP_OEQ;
                if (std::strcmp(op, "<") == 0)
                    pred = llvm::CmpInst::FCMP_OLT;
                else if (std::strcmp(op, ">") == 0)
                    pred = llvm::CmpInst::FCMP_OGT;
                else if (std::strcmp(op, "<=") == 0)
                    pred = llvm::CmpInst::FCMP_OLE;
                else if (std::strcmp(op, ">=") == 0)
                    pred = llvm::CmpInst::FCMP_OGE;
                else if (std::strcmp(op, "!=") == 0)
                    pred = llvm::CmpInst::FCMP_UNE;
                out.v = builder.CreateFCmp(pred, left.v, right.v);
            } else {
                llvm::CmpInst::Predicate pred = llvm::CmpInst::ICMP_EQ;
                bool sig = dest.is_signed;
                if (std::strcmp(op, "<") == 0)
                    pred = sig ? llvm::CmpInst::ICMP_SLT : llvm::CmpInst::ICMP_ULT;
                else if (std::strcmp(op, ">") == 0)
                    pred = sig ? llvm::CmpInst::ICMP_SGT : llvm::CmpInst::ICMP_UGT;
                else if (std::strcmp(op, "<=") == 0)
                    pred = sig ? llvm::CmpInst::ICMP_SLE : llvm::CmpInst::ICMP_ULE;
                else if (std::strcmp(op, ">=") == 0)
                    pred = sig ? llvm::CmpInst::ICMP_SGE : llvm::CmpInst::ICMP_UGE;
                else if (std::strcmp(op, "!=") == 0)
                    pred = llvm::CmpInst::ICMP_NE;
                out.v = builder.CreateICmp(pred, left.v, right.v);
            }
            return out;
        }
        out.ty = dest;
        if (dest.is_float) {
            if (std::strcmp(op, "+") == 0)
                out.v = builder.CreateFAdd(left.v, right.v);
            else if (std::strcmp(op, "-") == 0)
                out.v = builder.CreateFSub(left.v, right.v);
            else if (std::strcmp(op, "*") == 0)
                out.v = builder.CreateFMul(left.v, right.v);
            else if (std::strcmp(op, "/") == 0)
                out.v = builder.CreateFDiv(left.v, right.v);
            else if (std::strcmp(op, "%") == 0)
                out.v = builder.CreateFRem(left.v, right.v);
            else {
                error("not in this phase");
                return fallback();
            }
            return out;
        }
        if (std::strcmp(op, "+") == 0)
            out.v = builder.CreateAdd(left.v, right.v);
        else if (std::strcmp(op, "-") == 0)
            out.v = builder.CreateSub(left.v, right.v);
        else if (std::strcmp(op, "*") == 0)
            out.v = builder.CreateMul(left.v, right.v);
        else if (std::strcmp(op, "/") == 0)
            out.v = dest.is_signed ? builder.CreateSDiv(left.v, right.v)
                                   : builder.CreateUDiv(left.v, right.v);
        else if (std::strcmp(op, "%") == 0)
            out.v = dest.is_signed ? builder.CreateSRem(left.v, right.v)
                                   : builder.CreateURem(left.v, right.v);
        else if (std::strcmp(op, "&") == 0)
            out.v = builder.CreateAnd(left.v, right.v);
        else if (std::strcmp(op, "|") == 0)
            out.v = builder.CreateOr(left.v, right.v);
        else if (std::strcmp(op, "^") == 0)
            out.v = builder.CreateXor(left.v, right.v);
        else {
            error("not in this phase");
            return fallback();
        }
        return out;
    }

    RValue emit_assign(const AstNode *node)
    {
        std::optional<Slot> slot = lvalue(node->a);
        if (!slot || failed)
            return fallback();
        if (slot->ty.is_array) {
            error("cannot assign an array");
            return fallback();
        }
        if (node->text == NULL) {
            error("not in this phase");
            return fallback();
        }
        RValue stored;
        if (std::strcmp(node->text, "=") == 0) {
            RValue value = emit_rvalue(node->b);
            if (failed)
                return fallback();
            stored = cast_to(value, slot->ty);
        } else {
            std::string op = node->text;
            if (op.empty() || op.back() != '=') {
                error("not in this phase");
                return fallback();
            }
            op.pop_back();
            RValue left;
            left.v = load_slot(*slot);
            left.ty = drop(slot->ty);
            RValue right = emit_rvalue(node->b);
            if (failed)
                return fallback();
            RValue combined = apply_op(op.c_str(), left, right);
            if (failed)
                return fallback();
            stored = cast_to(combined, slot->ty);
        }
        store_slot(*slot, stored.v, false);
        stored.ty = drop(slot->ty);
        return stored;
    }

    RValue emit_conditional(const AstNode *node)
    {
        Ty left_ty = type_of(node->b);
        Ty right_ty = type_of(node->c);
        if (failed)
            return fallback();
        if (left_ty.is_void || right_ty.is_void) {
            error("conditional operand is void");
            return fallback();
        }
        Ty dest = usual(left_ty, right_ty);
        llvm::Value *cond = emit_bool(node->a);
        if (failed)
            return fallback();
        llvm::BasicBlock *then_block = new_block("cond.then");
        llvm::BasicBlock *else_block = new_block("cond.else");
        llvm::BasicBlock *end_block = new_block("cond.end");
        builder.CreateCondBr(cond, then_block, else_block);
        builder.SetInsertPoint(then_block);
        RValue then_value = cast_to(emit_rvalue(node->b), dest);
        if (failed)
            return fallback();
        llvm::BasicBlock *then_end = builder.GetInsertBlock();
        builder.CreateBr(end_block);
        builder.SetInsertPoint(else_block);
        RValue else_value = cast_to(emit_rvalue(node->c), dest);
        if (failed)
            return fallback();
        llvm::BasicBlock *else_end = builder.GetInsertBlock();
        builder.CreateBr(end_block);
        builder.SetInsertPoint(end_block);
        llvm::PHINode *phi = builder.CreatePHI(dest.ir, 2);
        phi->addIncoming(then_value.v, then_end);
        phi->addIncoming(else_value.v, else_end);
        return RValue{phi, drop(dest)};
    }

    void emit_statement(const AstNode *node)
    {
        if (failed || node == NULL)
            return;
        switch (node->kind) {
        case AST_LABEL:
            emit_label(node);
            return;
        case AST_CASE:
            emit_case(node);
            return;
        case AST_DEFAULT:
            emit_default(node);
            return;
        case AST_COMPOUND:
            emit_compound(node);
            return;
        default:
            break;
        }
        ensure_insert();
        if (failed)
            return;
        switch (node->kind) {
        case AST_EXPR_STMT:
            if (node->a != NULL)
                emit_rvalue(node->a);
            break;
        case AST_DECLARATION:
            emit_declaration(node, false);
            break;
        case AST_IF:
            emit_if(node);
            break;
        case AST_SWITCH:
            emit_switch(node);
            break;
        case AST_WHILE:
            emit_while(node);
            break;
        case AST_DO:
            emit_do(node);
            break;
        case AST_FOR:
            emit_for(node);
            break;
        case AST_GOTO:
            emit_goto(node);
            break;
        case AST_CONTINUE:
            emit_continue();
            break;
        case AST_BREAK:
            emit_break();
            break;
        case AST_RETURN:
            emit_return(node);
            break;
        case AST_USING:
            error("namespaces are a later phase");
            break;
        default:
            reject_node(node);
            break;
        }
    }

    void emit_items(const AstNode *item)
    {
        for (; item != NULL && !failed; item = item->next) {
            if (item->kind == AST_DECLARATION)
                emit_declaration(item, false);
            else if (item->kind == AST_USING)
                add_using(item);
            else if (item->kind == AST_NAMESPACE)
                error("namespace is not allowed here");
            else
                emit_statement(item);
        }
    }

    void emit_compound(const AstNode *node)
    {
        push_scope();
        emit_items(node->a);
        pop_scope();
    }

    void emit_if(const AstNode *node)
    {
        llvm::Value *cond = emit_bool(node->a);
        if (failed)
            return;
        llvm::BasicBlock *then_block = new_block("if.then");
        llvm::BasicBlock *else_block = new_block("if.else");
        llvm::BasicBlock *merge_block = node->c != NULL ? new_block("if.end") : else_block;
        builder.CreateCondBr(cond, then_block, else_block);
        builder.SetInsertPoint(then_block);
        emit_statement(node->b);
        if (!failed && !terminated())
            builder.CreateBr(merge_block);
        if (node->c != NULL) {
            builder.SetInsertPoint(else_block);
            emit_statement(node->c);
            if (!failed && !terminated())
                builder.CreateBr(merge_block);
        }
        builder.SetInsertPoint(merge_block);
    }

    bool case_key(const AstNode *expr, const Ty &dest, uint64_t &key, llvm::ConstantInt *&iconst)
    {
        Const value = eval_const(expr);
        if (!value.ok) {
            if (!failed)
                error("case is not constant");
            return false;
        }
        if (value.ty.is_float || value.ty.is_void || dest.ir == NULL || !dest.ir->isIntegerTy()) {
            error("case is not an integer");
            return false;
        }
        llvm::Constant *casted = convert_const(value.v, value.ty, dest);
        iconst = llvm::dyn_cast<llvm::ConstantInt>(casted);
        if (iconst == NULL) {
            error("case is not an integer");
            return false;
        }
        key = iconst->getZExtValue();
        return !failed;
    }

    void collect_cases(const AstNode *node)
    {
        for (const AstNode *item = node; item != NULL && !failed; item = item->next)
            collect_one(item);
    }

    void collect_one(const AstNode *node)
    {
        if (node == NULL || failed || collecting == NULL)
            return;
        if (node->kind == AST_SWITCH)
            return;
        if (node->kind == AST_CASE) {
            uint64_t key = 0;
            llvm::ConstantInt *iconst = nullptr;
            if (!case_key(node->a, collecting->ty, key, iconst))
                return;
            if (collecting->cases.count(key) != 0) {
                error("duplicate case");
                return;
            }
            collecting->cases[key] = CaseDest{iconst, nullptr};
            collect_one(node->b);
            return;
        }
        if (node->kind == AST_DEFAULT) {
            if (collecting->saw_default) {
                error("duplicate default");
                return;
            }
            collecting->saw_default = true;
            collect_one(node->a);
            return;
        }
        collect_cases(node->a);
        collect_cases(node->b);
        collect_cases(node->c);
        collect_cases(node->d);
    }

    void emit_switch(const AstNode *node)
    {
        RValue cond = emit_rvalue(node->a);
        if (failed)
            return;
        if (cond.ty.is_float || cond.ty.is_void) {
            error("switch condition is not an integer");
            return;
        }
        Ty promoted = promote(cond.ty);
        llvm::Value *value = convert(cond.v, cond.ty, promoted);
        SwitchFrame frame;
        frame.ty = promoted;
        SwitchFrame *saved = collecting;
        collecting = &frame;
        collect_cases(node->b);
        collecting = saved;
        if (failed)
            return;
        llvm::BasicBlock *exit_block = new_block("switch.exit");
        if (frame.saw_default)
            frame.default_bb = new_block("switch.default");
        for (auto &arm : frame.cases)
            arm.second.block = new_block("switch.case");
        llvm::SwitchInst *inst = builder.CreateSwitch(
            value, frame.default_bb != NULL ? frame.default_bb : exit_block,
            static_cast<unsigned>(frame.cases.size()));
        for (auto &arm : frame.cases)
            inst->addCase(arm.second.value, arm.second.block);
        loops.push_back(LoopFrame{exit_block, nullptr});
        switches.push_back(frame);
        emit_statement(node->b);
        if (!failed && !terminated())
            builder.CreateBr(exit_block);
        switches.pop_back();
        loops.pop_back();
        if (!failed)
            builder.SetInsertPoint(exit_block);
    }

    void emit_case(const AstNode *node)
    {
        if (switches.empty()) {
            error("case is outside a switch");
            return;
        }
        uint64_t key = 0;
        llvm::ConstantInt *iconst = nullptr;
        if (!case_key(node->a, switches.back().ty, key, iconst))
            return;
        auto arm = switches.back().cases.find(key);
        if (arm == switches.back().cases.end() || arm->second.block == NULL) {
            error("case is not constant");
            return;
        }
        if (!terminated())
            builder.CreateBr(arm->second.block);
        builder.SetInsertPoint(arm->second.block);
        emit_statement(node->b);
    }

    void emit_default(const AstNode *node)
    {
        if (switches.empty() || switches.back().default_bb == NULL) {
            error("default is outside a switch");
            return;
        }
        llvm::BasicBlock *dest = switches.back().default_bb;
        if (!terminated())
            builder.CreateBr(dest);
        builder.SetInsertPoint(dest);
        emit_statement(node->a);
    }

    void emit_while(const AstNode *node)
    {
        llvm::BasicBlock *cond_block = new_block("while.cond");
        llvm::BasicBlock *body_block = new_block("while.body");
        llvm::BasicBlock *exit_block = new_block("while.exit");
        builder.CreateBr(cond_block);
        builder.SetInsertPoint(cond_block);
        llvm::Value *cond = emit_bool(node->a);
        if (failed)
            return;
        builder.CreateCondBr(cond, body_block, exit_block);
        loops.push_back(LoopFrame{exit_block, cond_block});
        builder.SetInsertPoint(body_block);
        emit_statement(node->b);
        if (!failed && !terminated())
            builder.CreateBr(cond_block);
        loops.pop_back();
        if (!failed)
            builder.SetInsertPoint(exit_block);
    }

    void emit_do(const AstNode *node)
    {
        llvm::BasicBlock *body_block = new_block("do.body");
        llvm::BasicBlock *cond_block = new_block("do.cond");
        llvm::BasicBlock *exit_block = new_block("do.exit");
        builder.CreateBr(body_block);
        loops.push_back(LoopFrame{exit_block, cond_block});
        builder.SetInsertPoint(body_block);
        emit_statement(node->a);
        if (!failed && !terminated())
            builder.CreateBr(cond_block);
        loops.pop_back();
        if (failed)
            return;
        builder.SetInsertPoint(cond_block);
        llvm::Value *cond = emit_bool(node->b);
        if (failed)
            return;
        builder.CreateCondBr(cond, body_block, exit_block);
        builder.SetInsertPoint(exit_block);
    }

    void emit_for(const AstNode *node)
    {
        bool has_decl = node->a != NULL && node->a->kind == AST_DECLARATION;
        if (has_decl)
            push_scope();
        if (node->a != NULL) {
            if (has_decl)
                emit_declaration(node->a, false);
            else {
                ensure_insert();
                emit_rvalue(node->a);
            }
        }
        if (failed) {
            if (has_decl)
                pop_scope();
            return;
        }
        llvm::BasicBlock *cond_block = new_block("for.cond");
        llvm::BasicBlock *body_block = new_block("for.body");
        llvm::BasicBlock *step_block = new_block("for.step");
        llvm::BasicBlock *exit_block = new_block("for.exit");
        builder.CreateBr(cond_block);
        builder.SetInsertPoint(cond_block);
        if (node->b == NULL)
            builder.CreateBr(body_block);
        else {
            llvm::Value *cond = emit_bool(node->b);
            if (!failed)
                builder.CreateCondBr(cond, body_block, exit_block);
        }
        if (failed) {
            if (has_decl)
                pop_scope();
            return;
        }
        loops.push_back(LoopFrame{exit_block, step_block});
        builder.SetInsertPoint(body_block);
        emit_statement(node->d);
        if (!failed && !terminated())
            builder.CreateBr(step_block);
        loops.pop_back();
        if (!failed) {
            builder.SetInsertPoint(step_block);
            if (node->c != NULL)
                emit_rvalue(node->c);
            if (!failed && !terminated())
                builder.CreateBr(cond_block);
        }
        if (has_decl)
            pop_scope();
        if (!failed)
            builder.SetInsertPoint(exit_block);
    }

    void emit_label(const AstNode *node)
    {
        if (node->text == NULL) {
            error("label has no name");
            return;
        }
        LabelInfo &label = labels[node->text];
        if (label.defined) {
            error_name("duplicate label", node->text);
            return;
        }
        if (label.bb == NULL)
            label.bb = new_block(node->text);
        label.defined = true;
        if (!terminated())
            builder.CreateBr(label.bb);
        builder.SetInsertPoint(label.bb);
        if (node->a != NULL)
            emit_statement(node->a);
    }

    void emit_goto(const AstNode *node)
    {
        if (node->text == NULL) {
            error("goto has no name");
            return;
        }
        LabelInfo &label = labels[node->text];
        if (label.bb == NULL)
            label.bb = new_block(node->text);
        builder.CreateBr(label.bb);
    }

    void emit_break()
    {
        if (loops.empty() || loops.back().break_bb == NULL) {
            error("break is outside a loop or switch");
            return;
        }
        builder.CreateBr(loops.back().break_bb);
    }

    void emit_continue()
    {
        for (auto frame = loops.rbegin(); frame != loops.rend(); ++frame) {
            if (frame->continue_bb == NULL)
                continue;
            builder.CreateBr(frame->continue_bb);
            return;
        }
        error("continue is outside a loop");
    }

    void emit_return(const AstNode *node)
    {
        if (ret_ty.is_void) {
            if (node->a != NULL) {
                RValue value = emit_rvalue(node->a);
                if (!failed && !value.ty.is_void)
                    error("void function returns a value");
            }
            if (!failed && !terminated())
                builder.CreateRetVoid();
            return;
        }
        if (node->a == NULL) {
            error("return needs a value");
            return;
        }
        RValue value = cast_to(emit_rvalue(node->a), ret_ty);
        if (!failed && !terminated())
            builder.CreateRet(value.v);
    }

    const AstNode *scalar_declarator(const AstNode *node, const AstNode *&init)
    {
        init = NULL;
        const AstNode *declarator = node;
        if (node != NULL && node->kind == AST_INIT) {
            declarator = node->a;
            init = node->b;
            if (init != NULL && init->kind == AST_INIT) {
                if (init->a == NULL || init->a->next != NULL || init->a->kind == AST_DESIGNATED) {
                    error("aggregates are a later phase");
                    return NULL;
                }
                init = init->a;
            }
        }
        if (declarator == NULL || declarator->kind != AST_IDENT || declarator->text == NULL) {
            reject_declarator(declarator);
            return NULL;
        }
        return declarator;
    }

    bool is_string(const AstNode *node)
    {
        return node != NULL && node->kind == AST_LITERAL && node->text != NULL
            && (node->text[0] == '"' || (node->text[0] == 'L' && node->text[1] == '"'));
    }

    Ty string_ty()
    {
        Ty ty = pointer_to(integer(8, true, 1));
        return ty;
    }

    bool decode_string(const char *text, std::string &bytes)
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

    RValue emit_string(const char *text)
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

    bool same_value_type(Ty left, Ty right)
    {
        left = drop(left);
        right = drop(right);
        if (left.is_pointer || right.is_pointer) {
            return left.is_pointer && right.is_pointer && left.inner != NULL && right.inner != NULL
                && same_value_type(*left.inner, *right.inner);
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

    bool storage_conflict(const Storage &storage)
    {
        int classes = static_cast<int>(storage.is_typedef) + static_cast<int>(storage.is_extern)
            + static_cast<int>(storage.is_static) + static_cast<int>(storage.is_register);
        if (classes > 1)
            return true;
        return storage.is_inline && (storage.is_typedef || storage.is_register);
    }

    std::string type_tag(const Ty &ty)
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

    std::string sig_text(const Func &fn)
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

    void rename_group(std::vector<Func> &group)
    {
        for (Func &fn : group)
            fn.fn->setName(sig_text(fn));
    }

    bool same_signature(const Func &left, const Func &right)
    {
        if (left.vararg != right.vararg || left.params.size() != right.params.size())
            return false;
        for (std::size_t i = 0; i < left.params.size(); i++) {
            if (!same_value_type(left.params[i], right.params[i]))
                return false;
        }
        return true;
    }

    void install(Func incoming)
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

    Func *resolve_keys(const std::vector<std::string> &keys, const std::vector<Ty> &args, const char *label)
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

    Func *resolve(const char *name, const std::vector<Ty> &args)
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

    Ty vararg_ty(Ty ty)
    {
        if (ty.is_float && ty.fp_rank < 2)
            return floating(true);
        return promote(ty);
    }

    const AstNode *function_declarator(const AstNode *node)
    {
        if (node != NULL && node->kind == AST_INIT)
            node = node->a;
        if (node != NULL && node->kind == AST_FUNCTION_DECL)
            return node;
        return NULL;
    }

    bool call_target(const AstNode *callee, std::vector<std::string> &keys, const char *&label)
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

    Ty type_call(const AstNode *node)
    {
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

    RValue emit_call(const AstNode *node)
    {
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

    void record_alias(const std::string &name, Ty ty)
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

    bool plan_record(const Record &rec, const AstNode *list, std::vector<const AstNode *> &out)
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

    bool plan_array(const Ty &ty, const AstNode *list, std::vector<const AstNode *> &out)
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

    bool union_choice(const Record &rec, const AstNode *list, int &index, const AstNode *&value)
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

    llvm::Constant *field_constant(const Field &field, const AstNode *init)
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

    llvm::Constant *const_initializer(const Ty &ty, const AstNode *init)
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

    void store_initializer(const Slot &slot, const AstNode *init)
    {
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

    void place_value(const std::string &name, Ty ty, llvm::Value *addr)
    {
        if (ty.is_union) {
            if (auto *global = llvm::dyn_cast<llvm::GlobalVariable>(addr))
                global->setAlignment(llvm::Align(record_align(ty)));
            else if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(addr))
                alloca->setAlignment(llvm::Align(record_align(ty)));
        }
        bind(name, Slot{addr, ty});
    }

    void emit_declaration(const AstNode *node, bool file)
    {
        Storage storage;
        Ty ty = parse_specs(node->a, file ? Where::File : Where::Local, &storage);
        if (failed)
            return;
        if (storage_conflict(storage)) {
            error("invalid storage class");
            return;
        }
        if (file && storage.is_register) {
            error("register is not allowed here");
            return;
        }
        if (storage.is_typedef) {
            if (!file) {
                error("typedef is not allowed here");
                return;
            }
            if (node->b == NULL) {
                error("typedef has no name");
                return;
            }
            for (const AstNode *item = node->b; item != NULL && !failed; item = item->next) {
                Declared declared = declare_one(ty, item);
                if (failed)
                    return;
                if (declared.func != NULL) {
                    error("not a variable");
                    return;
                }
                if (declared.init != NULL) {
                    error("typedef has an initializer");
                    return;
                }
                if (declared.name == NULL || declared.name->kind != AST_IDENT
                    || declared.name->text == NULL) {
                    error("typedef has no name");
                    return;
                }
                record_alias(qualify(declared.name->text), declared.ty);
            }
            return;
        }
        if (node->b == NULL) {
            if (ty.is_record || ty.is_enum)
                return;
            error("declaration has no name");
            return;
        }
        for (const AstNode *item = node->b; item != NULL && !failed; item = item->next) {
            Declared declared = declare_one(ty, item);
            if (failed)
                return;
            if (declared.func != NULL) {
                if (declared.init != NULL) {
                    error("function has an initializer");
                    return;
                }
                if (!file && !storage.is_extern) {
                    error("a nested function is not allowed");
                    return;
                }
                if (!file && storage.is_static) {
                    error("static is not allowed here");
                    return;
                }
                const AstNode *decl = item->kind == AST_INIT ? item->a : item;
                declare_function(node->a, decl, NULL);
                continue;
            }
            if (storage.is_inline) {
                error("inline variable");
                return;
            }
            if (!file && (storage.is_extern || storage.is_static)) {
                error("storage class is not allowed here");
                return;
            }
            if (declared.ty.is_void) {
                error("void variable");
                return;
            }
            if (!complete_type(declared.ty)) {
                error("incomplete type");
                return;
            }
            if (declared.name == NULL || declared.name->kind != AST_IDENT
                || declared.name->text == NULL) {
                if (declared.name != NULL && declared.name->kind == AST_QUALIFIED)
                    error("qualified names are a later phase");
                else
                    error("declaration has no name");
                return;
            }
            std::string name = declared.name->text;
            if (file)
                name = qualify(name);
            Ty object = declared.ty;
            if (file) {
                if (scopes.front().count(name) != 0 || overloads.count(name) != 0
                    || aliases.count(name) != 0 || enum_consts.count(name) != 0
                    || namespaces.count(name) != 0) {
                    error_name("name is already defined", name.c_str());
                    return;
                }
                bool declaration_only = storage.is_extern && declared.init == NULL;
                auto linkage = storage.is_static ? llvm::GlobalValue::InternalLinkage
                                                 : llvm::GlobalValue::ExternalLinkage;
                llvm::Constant *initializer = NULL;
                if (!declaration_only) {
                    if (declared.init == NULL)
                        initializer = llvm::Constant::getNullValue(object.ir);
                    else
                        initializer = const_initializer(object, declared.init);
                }
                auto *global = new llvm::GlobalVariable(
                    module, object.ir, object.is_const, linkage, initializer, name);
                place_value(name, object, global);
            } else {
                if (declared.init != NULL)
                    ensure_insert();
                llvm::AllocaInst *addr = alloca_entry(object.ir, name);
                Slot slot{addr, object};
                if (object.is_union)
                    addr->setAlignment(llvm::Align(record_align(object)));
                bind(name, slot);
                if (declared.init != NULL)
                    store_initializer(slot, declared.init);
            }
        }
    }

    bool collect_params(const AstNode *list, std::vector<Param> &out, bool &vararg)
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

    void declare_function(const AstNode *specs, const AstNode *decl, const AstNode *body)
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
        if (declared.func->c != NULL) {
            error("method qualifiers are a later phase");
            return;
        }
        if (body != NULL && body->kind != AST_COMPOUND) {
            error("function has no body");
            return;
        }
        if (declared.name == NULL || declared.name->kind != AST_IDENT || declared.name->text == NULL) {
            if (declared.name != NULL && declared.name->kind == AST_QUALIFIED)
                error("qualified names are a later phase");
            else
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

    void emit_body(Func &fn)
    {
        scopes.resize(1);
        blocks.clear();
        labels.clear();
        loops.clear();
        switches.clear();
        std::vector<std::string> saved_ns = ns_stack;
        const std::vector<LookupScope> *saved_lookup = active_lookup;
        ns_stack = fn.home;
        active_lookup = &fn.lookup;
        function = fn.fn;
        ret_ty = fn.ret;
        entry = llvm::BasicBlock::Create(context, "entry", function);
        builder.SetInsertPoint(entry);
        push_scope();
        auto argument = function->arg_begin();
        for (std::size_t i = 0; i < fn.params.size(); i++) {
            const std::string &name = fn.names[i];
            if (!name.empty())
                argument->setName(name);
            llvm::AllocaInst *addr = alloca_entry(fn.params[i].ir, name.empty() ? "param" : name);
            Slot slot{addr, fn.params[i]};
            if (!name.empty())
                bind(name, slot);
            store_slot(slot, &*argument, true);
            ++argument;
        }
        emit_items(fn.body->a);
        if (!failed) {
            for (const auto &label : labels) {
                if (!label.second.defined) {
                    error_name("undefined label", label.first.c_str());
                    break;
                }
            }
        }
        if (!failed) {
            for (llvm::BasicBlock &block : *function) {
                if (block.getTerminator() != NULL)
                    continue;
                builder.SetInsertPoint(&block);
                if (ret_ty.is_void)
                    builder.CreateRetVoid();
                else
                    builder.CreateUnreachable();
            }
        }
        pop_scope();
        scopes.resize(1);
        blocks.clear();
        ns_stack = saved_ns;
        active_lookup = saved_lookup;
        function = nullptr;
        entry = nullptr;
    }
};

int compile_to_bitcode(const AstNode *root, const char *bitcode_path)
{
    llvm::LLVMContext context;
    auto module = std::make_unique<llvm::Module>("smallcpp", context);
    Codegen codegen(context, *module);
    if (codegen.run(root) != 0)
        return 1;

    std::string diagnostic;
    llvm::raw_string_ostream errors(diagnostic);
    if (llvm::verifyModule(*module, &errors)) {
        errors.flush();
        std::fprintf(stderr, "module failed verification: %s\n", diagnostic.c_str());
        return 1;
    }

    std::error_code error;
    llvm::raw_fd_ostream out(bitcode_path, error, llvm::sys::fs::OF_None);
    if (error) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, error.message().c_str());
        return 1;
    }
    llvm::WriteBitcodeToFile(*module, out);
    out.flush();
    if (out.has_error()) {
        std::fprintf(stderr, "%s: %s\n", bitcode_path, out.error().message().c_str());
        return 1;
    }
    return 0;
}
