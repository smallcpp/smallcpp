#pragma once

/*
 * Types shared by the IR emitter. Each phase file defines
 * the Codegen methods for that phase. codegen.cpp runs them.
 */
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
#include "llvm/IR/Intrinsics.h"
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

struct FnSig;

struct Ty {
    llvm::Type *ir = nullptr;
    bool is_signed = false;
    bool is_float = false;
    bool is_bool = false;
    bool is_void = false;
    bool is_const = false;
    bool is_volatile = false;
    bool is_pointer = false;
    bool is_null = false;
    bool is_array = false;
    bool is_record = false;
    bool is_union = false;
    bool is_enum = false;
    bool is_function = false;
    bool func_vararg = false;
    int array_len = 0;
    int rank = 0;
    int fp_rank = 0;
    std::string record;
    std::shared_ptr<Ty> inner;
    std::shared_ptr<FnSig> sig;
};

struct FnSig {
    std::vector<Ty> params;
};

struct Storage {
    bool is_typedef = false;
    bool is_extern = false;
    bool is_static = false;
    bool is_inline = false;
    bool is_register = false;
    bool is_virtual = false;
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
    std::string access = "public";
};

struct MethodDecl {
    std::string name;
    std::string ir_name;
    std::string access = "public";
    bool is_const = false;
    bool is_virtual = false;
    bool is_ctor = false;
    bool is_dtor = false;
    bool is_pure = false;
    int vindex = -1;
};

struct VSlot {
    std::string name;
    std::string vkey;
    std::string impl_class;
    int index = 0;
    bool pure = false;
    llvm::Function *fn = nullptr;
};

struct VBTable {
    std::vector<std::string> path;
    llvm::GlobalVariable *global = nullptr;
};

struct VTable {
    std::string base;
    std::vector<std::string> path;
    llvm::Constant *global = nullptr;
};

struct Record {
    std::string name;
    bool is_union = false;
    bool is_class = false;
    bool complete = false;
    bool defining = false;
    llvm::StructType *ir = nullptr;
    std::vector<Field> fields;
    std::vector<std::string> bases;
    std::vector<std::string> direct_virtual;
    std::vector<std::string> vbases;
    std::vector<uint64_t> vbase_off;
    uint64_t complete_size = 0;
    std::vector<MethodDecl> methods;
    int vptr = -1;
    int vbptr = -1;
    std::string primary_base;
    bool abstract = false;
    llvm::Constant *vtable = nullptr;
    std::vector<VSlot> vslots;
    std::vector<VTable> vtables;
    std::vector<VBTable> vbtables;
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
    const AstNode *expr = nullptr;
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
    std::vector<const AstNode *> default_exprs;
    std::vector<std::string> names;
    bool vararg = false;
    Ty ret;
    const AstNode *body = nullptr;
    std::string debug_name;
    int line = 0;
    bool is_static = false;
    bool is_inline = false;
    std::vector<std::string> home;
    std::vector<LookupScope> lookup;
    std::string klass;
    bool method_const = false;
    bool is_ctor = false;
    bool is_dtor = false;
    bool is_pure = false;
    const AstNode *inits = nullptr;
    int vindex = -1;
};

class Codegen;

struct SourceNote {
    Codegen &cg;
    const AstNode *previous;

    SourceNote(Codegen &cg, const AstNode *node);
    ~SourceNote();
};

struct Declared {
    const AstNode *name = nullptr;
    const AstNode *func = nullptr;
    const AstNode *init = nullptr;
    Ty ty;
    int bits = 0;
};

namespace llvm {
class DIBuilder;
class DIFile;
class DIType;
}

class Codegen {
public:
    Codegen(llvm::LLVMContext &context, llvm::Module &module);
    ~Codegen();
    int run(const AstNode *root);
    void enable_debug(const char *path);
    void finish_debug();

private:
    llvm::LLVMContext &context;
    llvm::Module &module;
    llvm::IRBuilder<> builder;
    llvm::Function *function = nullptr;
    llvm::BasicBlock *entry = nullptr;
    Ty ret_ty;
    bool failed = false;
    const AstNode *focus = nullptr;
    const Ty *expect = nullptr;
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
    std::string method_class;
    bool method_const = false;
    Ty this_ty;
    std::vector<llvm::BasicBlock *> handlers;
    int in_handler = 0;
    std::vector<char> catch_open;
    std::vector<LoopFrame> loops;
    std::vector<SwitchFrame> switches;
    SwitchFrame *collecting = nullptr;
    std::map<std::string, LabelInfo> labels;

    struct MethodCall {
        std::string ir_name;
        std::string owner;
        bool qualified = false;
        const AstNode *object = NULL;
        bool arrow = false;
        bool implicit = false;
    };

    friend struct SourceNote;

    void set_focus(const AstNode *node);
    void begin_function_debug(Func &fn);
    void declare_param(llvm::AllocaInst *addr, const std::string &name, const Ty &ty, unsigned arg_no,
                       unsigned line);
    void declare_local(llvm::AllocaInst *addr, const std::string &name, const Ty &ty);
    llvm::DIType *debug_type(const Ty &ty);
    llvm::DIType *debug_record(const Ty &ty);
    std::string debug_basic_name(const Ty &ty) const;

    llvm::DIBuilder *di = nullptr;
    llvm::DIFile *di_file = nullptr;
    std::map<std::string, llvm::DIType *> di_types;
    llvm::StructType *info_layout = nullptr;

    void error(const char *message);
    void error_name(const char *message, const char *name);
    void note_storage(Storage *storage, bool already, bool *flag, const char *name);
    Ty drop(Ty ty);
    Ty integer(unsigned bits, bool is_signed, int rank);
    Ty floating(bool is_double);
    Ty bool_ty();
    Ty int_ty();
    Ty size_ty();
    Ty void_ty();
    RValue fallback();
    bool op_is(const AstNode *node, const char *text);
    bool is_integer(const Ty &ty);
    void reject_node(const AstNode *node);
    void reject_declarator(const AstNode *node);
    Ty pointer_to(Ty inner);
    Ty array_of(Ty elem, int length);
    Ty decay(Ty ty);
    Ty record_type(const Record &rec);
    Ty enum_type(const std::string &name);
    bool complete_type(const Ty &ty);
    unsigned abi_align(llvm::Type *ty);
    unsigned record_align(const Ty &ty);
    void note_quals(Ty &ty, const AstNode *list);
    Ty apply_pointers(Ty base, const AstNode *node);
    bool declarator_form(const AstNode *node);
    int const_i64(const AstNode *node, int64_t &out);
    Ty walk_declarator(Ty base, const AstNode *node, Declared &out);
    Declared declare_one(Ty base, const AstNode *node);
    std::string tag_key(bool is_union, const char *text);
    Ty make_record(const AstNode *node);
    Ty make_enum(const AstNode *node);
    std::string class_key_of(const char *plain) const;
    std::string source_name(const Record &rec) const;
    std::string base_field_name(const std::string &key) const;
    bool class_type(const Ty &ty) const;
    bool derives_from(const std::string &derived, const std::string &base) const;
    bool can_access(const std::string &access, const std::string &owner) const;
    Record *class_named(const char *plain);
    bool quals_const(const AstNode *list, bool &is_const);
    const char *node_name(const AstNode *node) const;
    void install_method(const std::string &key, MethodDecl decl, Ty ret, std::vector<Param> params,
                        const AstNode *body, const AstNode *inits);
    void layout_class(Record &rec);
    void build_vtable(const std::string &key);
    bool polymorphic(const std::string &key) const;
    bool vptr_at_start(const std::string &key) const;
    std::string virtual_key(const Ty &ret, const std::vector<Param> &params, bool is_const);
    std::string virtual_key(const Func &fn);
    bool hides_virtual(const std::string &base, const std::string &name, const std::string &vkey) const;
    Func *declared_virtual(const std::string &klass, const std::string &name, const std::string &vkey);
    Func *overrider_along(const std::string &klass, const std::vector<std::string> &path, std::size_t step,
                          const std::string &name, const std::string &vkey);
    uint64_t path_offset(const std::string &derived, const std::vector<std::string> &path) const;
    uint64_t thunk_delta(const std::string &complete, const std::vector<std::string> &path,
                         const std::string &impl) const;
    llvm::Constant *emit_vtable(const std::string &name, const std::vector<llvm::Constant *> &slots,
                                llvm::Constant *info);
    llvm::StructType *typeinfo_layout();
    Ty typeid_type();
    std::string describe_type(const Ty &ty, bool top, bool readable) const;
    llvm::Constant *type_descriptor(const Ty &ty);
    llvm::Value *dynamic_type(llvm::Value *object, const std::string &key);
    RValue emit_typeid(const AstNode *node);
    llvm::Function *make_thunk(llvm::Function *impl, uint64_t delta, const std::string &name);
    llvm::Value *base_ptr(llvm::Value *ptr, const std::string &from, const std::string &base);
    llvm::Value *vptr_addr(llvm::Value *ptr, const std::string &key);
    llvm::Constant *view_for(const std::string &complete, const std::vector<std::string> &path);
    void lay_bases(llvm::Value *ptr, const std::string &complete, const std::string &sub,
                   std::vector<std::string> &path);
    void build_views(const std::string &complete, const std::string &sub, std::vector<std::string> &path);
    VTable make_view(const std::string &complete, const std::string &base, const std::vector<std::string> &path);
    void emit_vptrs(llvm::Value *ptr, const std::string &key);
    void collect_vbases(const std::string &key, std::vector<std::string> &out,
                        std::set<std::string> &seen) const;
    void place_vbases(const std::string &key);
    bool field_path(const std::string &complete, const std::vector<std::string> &path) const;
    uint64_t subobject_at(const std::string &complete, const std::vector<std::string> &path) const;
    uint64_t vbase_at(const std::string &complete, const std::string &base) const;
    void add_vbtable(const std::string &complete, const std::string &sub, const std::vector<std::string> &path);
    void build_vbtables(const std::string &complete, const std::string &sub, std::vector<std::string> &path);
    llvm::GlobalVariable *vbtable_for(const std::string &complete, const std::vector<std::string> &path);
    void store_vbptrs(llvm::Value *complete_ptr, llvm::Value *sub_ptr, const std::string &complete,
                      const std::string &sub, std::vector<std::string> &path);
    void emit_vbptrs(llvm::Value *self, const std::string &key);
    void emit_virtual_bases(llvm::Value *self, const std::string &key, const AstNode *inits);
    bool has_constructor(const std::string &key);
    bool virtual_destructor(const std::string &key) const;
    bool overrides_slot(const std::string &key, const VSlot &slot);
    llvm::Value *vbase_addr(llvm::Value *ptr, const std::string &from, const std::string &base);
    void emit_pure_stub(Func &fn);
    llvm::Value *ctor_flag(bool most);
    Ty make_class(const AstNode *node);
    std::optional<Field> find_field(const Ty &object, const char *name);
    std::optional<Slot> member_slot(llvm::Value *object, const Ty &type, const char *name, bool report);
    std::optional<Ty> member_type(const Ty &type, const char *name);
    std::optional<Ty> implicit_member_type(const char *name);
    std::optional<Slot> implicit_member(const char *name);
    std::string method_owner(const std::string &klass, const char *name, std::string &ir_name);
    Ty parse_specs(const AstNode *list, Where where, Storage *storage = nullptr);
    bool is_type_form(const AstNode *node);
    Ty type_from_name(const AstNode *node);
    Ty promote(Ty ty);
    Ty usual(Ty left, Ty right);
    bool suffix_is(const std::string &spell, const char *suffix);
    bool decode_char(const char *text, unsigned &code);
    bool looks_float(const char *text);
    bool decode_literal(const char *text, Ty &ty, llvm::Constant *&value);
    llvm::Constant *convert_const(llvm::Constant *value, const Ty &from, const Ty &to);
    llvm::Value *convert(llvm::Value *value, const Ty &from, const Ty &to);
    RValue cast_to(RValue value, Ty dest);
    void push_scope();
    void pop_scope();
    std::string join_ns(int count) const;
    std::string current_path() const;
    std::string qualify(const std::string &name) const;
    bool name_known(const std::string &key) const;
    std::vector<std::string> dedup_keys(std::vector<std::string> keys) const;
    std::vector<std::string> use_keys(const std::vector<std::pair<std::string, std::string>> &names,
                                      const std::vector<std::string> &directives, const std::string &name) const;
    std::vector<std::string> scope_keys(const LookupScope &scope, const std::string &name) const;
    std::vector<LookupScope> live_chain() const;
    std::vector<std::string> visible_keys(const std::string &name) const;
    void flatten_name(const AstNode *node, std::vector<const AstNode *> &parts) const;
    std::string find_ns_component(const std::string &component);
    std::string namespace_key(const AstNode *node);
    std::string entity_key(const AstNode *node);
    bool namespace_only_name(const char *name) const;
    bool namespace_only(const std::string &key) const;
    std::optional<int64_t> lookup_enum(const char *name);
    std::optional<Ty> lookup_alias(const char *name);
    void declare_item(const AstNode *item);
    void declare_namespace(const AstNode *node);
    void add_using(const AstNode *node);
    std::optional<Slot> lookup(const char *name);
    void bind(const std::string &name, Slot slot);
    llvm::AllocaInst *alloca_entry(llvm::Type *ty, const std::string &name);
    llvm::Value *load_slot(const Slot &slot);
    void store_slot(const Slot &slot, llvm::Value *value, bool initializing);
    RValue read_slot(const Slot &slot);
    Slot field_addr(llvm::Value *base, const Ty &object, const Field &field);
    Ty object_type(const AstNode *node);
    std::optional<Slot> index_slot(const AstNode *node);
    bool terminated();
    void ensure_insert();
    llvm::BasicBlock *new_block(const char *name);
    std::optional<Slot> lvalue(const AstNode *node);
    Ty type_of(const AstNode *node);
    Ty type_unary(const AstNode *node);
    Ty type_binary(const AstNode *node);
    Const eval_const(const AstNode *node);
    Const eval_sizeof(const AstNode *operand);
    Const with_const(llvm::Constant *value, Ty ty);
    Const eval_unary(const AstNode *node);
    bool const_true(const Const &value);
    Const eval_binary(const AstNode *node);
    Const fold_int(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare);
    Const fold_float(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare);
    Const eval_conditional(const AstNode *node);
    Ty null_ty();
    llvm::Function *allocator();
    llvm::Function *set_dtor_fn();
    llvm::Function *dtor_done_fn();
    Func *destructor_of(const Ty &ty);
    void bind_destructor(llvm::Value *object, const Ty &ty);
    void emit_delete(const AstNode *node);
    Ty new_element(const AstNode *node, std::vector<const AstNode *> &bounds);
    std::vector<llvm::Value *> live_pointers();
    llvm::Value *allocate(llvm::Value *bytes, llvm::Value *count);
    RValue emit_native(const AstNode *node);
    RValue emit_new(const AstNode *node);
    RValue emit_rvalue(const AstNode *node);
    RValue emit_cast(const AstNode *node);
    RValue emit_unary(const AstNode *node);
    RValue emit_inc(const AstNode *node);
    llvm::Value *emit_bool(const AstNode *node);
    RValue emit_and(const AstNode *left, const AstNode *right);
    RValue emit_or(const AstNode *left, const AstNode *right);
    RValue emit_binary(const AstNode *node);
    RValue pointer_binary(const char *op, RValue left, RValue right, bool compare);
    RValue apply_op(const char *op, RValue left, RValue right);
    RValue emit_assign(const AstNode *node);
    RValue emit_conditional(const AstNode *node);
    llvm::Function *declare_runtime(const char *name, llvm::FunctionType *type);
    llvm::Function *personality();
    llvm::Function *throw_fn();
    llvm::Function *rethrow_fn();
    llvm::Function *note_throw_fn();
    llvm::Function *allocate_exception();
    llvm::Function *begin_catch_fn();
    llvm::Function *end_catch_fn();
    llvm::Function *typeid_fn();
    void note_unwind();
    void close_open_catches();
    llvm::Constant *type_info(const Ty &ty);
    void emit_throw(const AstNode *node);
    void emit_try(const AstNode *node);
    void emit_statement(const AstNode *node);
    void emit_items(const AstNode *item);
    void emit_compound(const AstNode *node);
    void emit_if(const AstNode *node);
    bool case_key(const AstNode *expr, const Ty &dest, uint64_t &key, llvm::ConstantInt *&iconst);
    void collect_cases(const AstNode *node);
    void collect_one(const AstNode *node);
    void emit_switch(const AstNode *node);
    void emit_case(const AstNode *node);
    void emit_default(const AstNode *node);
    void emit_while(const AstNode *node);
    void emit_do(const AstNode *node);
    void emit_for(const AstNode *node);
    void emit_label(const AstNode *node);
    void emit_goto(const AstNode *node);
    void emit_break();
    void emit_continue();
    void emit_return(const AstNode *node);
    const AstNode *scalar_declarator(const AstNode *node, const AstNode *&init);
    bool is_string(const AstNode *node);
    Ty string_ty();
    bool decode_string(const char *text, std::string &bytes);
    RValue emit_string(const char *text);
    bool same_value_type(Ty left, Ty right);
    bool storage_conflict(const Storage &storage);
    std::string type_tag(const Ty &ty);
    std::string sig_text(const Func &fn);
    void rename_group(std::vector<Func> &group);
    bool same_signature(const Func &left, const Func &right);
    bool has_default(const Func &fn, unsigned index) const;
    void append_omitted(std::vector<llvm::Value *> &out, const Func &fn,
                        unsigned begin, unsigned end, int most_flag);
    void install(Func incoming);
    Func *implicit_extern(const char *name, const std::vector<Ty> &args);
    Func *resolve_keys(const std::vector<std::string> &keys, const std::vector<Ty> &args, const char *label);
    Func *resolve(const char *name, const std::vector<Ty> &args);
    Ty vararg_ty(Ty ty);
    const AstNode *function_declarator(const AstNode *node);
    bool call_target(const AstNode *callee, std::vector<std::string> &keys, const char *&label);
    bool prepare_method(const AstNode *callee, MethodCall &call);
    llvm::Value *adjust_to(llvm::Value *ptr, const std::string &from, const std::string &owner);
    const MethodDecl *method_decl(const std::string &owner, const std::string &ir_name) const;
    Func *pick_method(const MethodCall &call, const std::vector<Ty> &args, bool object_const, const char *label);
    Ty type_method_call(const AstNode *node, const MethodCall &call);
    RValue emit_method_call(const AstNode *node, const MethodCall &call);
    Ty type_call(const AstNode *node);
    RValue emit_call(const AstNode *node);
    void record_alias(const std::string &name, Ty ty);
    bool plan_record(const Record &rec, const AstNode *list, std::vector<const AstNode *> &out);
    bool plan_array(const Ty &ty, const AstNode *list, std::vector<const AstNode *> &out);
    bool union_choice(const Record &rec, const AstNode *list, int &index, const AstNode *&value);
    llvm::Constant *field_constant(const Field &field, const AstNode *init);
    llvm::Constant *const_initializer(const Ty &ty, const AstNode *init);
    void store_initializer(const Slot &slot, const AstNode *init);
    void place_value(const std::string &name, Ty ty, llvm::Value *addr);
    void emit_declaration(const AstNode *node, bool file);
    bool collect_params(const AstNode *list, std::vector<Param> &out, bool &vararg);
    void split_function(const AstNode *node, const AstNode *&inner, const AstNode *&params);
    Ty make_function(Ty ret, const std::vector<Param> &params, bool vararg);
    Ty function_from_decl(Ty ret, const AstNode *node);
    Ty signature_ty(const Func &fn);
    bool fn_pointer(const Ty &ty);
    bool function_keys(const AstNode *node, std::vector<std::string> &keys, const char *&label);
    Func *pick_address(const std::vector<std::string> &keys, const char *label);
    RValue take_function(const std::vector<std::string> &keys, const char *label);
    bool pointer_callee(const AstNode *callee, Ty &ptr_ty);
    RValue load_callee(const AstNode *callee);
    RValue emit_indirect(const AstNode *node, const Ty &ptr_ty);
    void declare_special(const AstNode *node);
    void declare_qualified_method(const Declared &declared, bool is_const, bool is_virtual,
                                  const AstNode *body);
    void declare_function(const AstNode *specs, const AstNode *decl, const AstNode *body);
    void emit_ctor_call(llvm::Value *self, const std::string &owner, const AstNode *args, bool most_derived);
    void emit_inits(Func &fn);
    void emit_body(Func &fn);
};

inline SourceNote::SourceNote(Codegen &cg, const AstNode *node)
    : cg(cg), previous(cg.focus)
{
    if (node != NULL && node->line > 0)
        cg.set_focus(node);
}

inline SourceNote::~SourceNote()
{
    cg.set_focus(previous);
}
