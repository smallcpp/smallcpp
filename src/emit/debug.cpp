/*
 * Debug info for -g. Without it, the emitter does not build a DIBuilder.
 */
#include "lower.hpp"

#include "llvm/BinaryFormat/Dwarf.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

static std::string debug_tag_name(const std::string &key)
{
    std::string name = key;
    const char *markers[] = {"struct.", "union.", "class.", "enum."};
    for (const char *marker : markers) {
        const auto at = name.rfind(marker);
        if (at == std::string::npos)
            continue;
        name = name.substr(0, at) + name.substr(at + std::char_traits<char>::length(marker));
        break;
    }
    if (!name.empty() && name[0] == '.')
        name.erase(name.begin());
    return name;
}

Codegen::~Codegen()
{
    delete di;
}

void Codegen::enable_debug(const char *path)
{
    if (path == nullptr || path[0] == '\0' || di != nullptr)
        return;
    llvm::SmallString<256> absolute(path);
    llvm::sys::fs::make_absolute(absolute);
    llvm::StringRef text(absolute);
    const std::size_t slash = text.find_last_of('/');
    llvm::StringRef directory = slash == llvm::StringRef::npos ? llvm::StringRef(".") : text.take_front(slash);
    llvm::StringRef filename = slash == llvm::StringRef::npos ? text : text.drop_front(slash + 1);
    di = new llvm::DIBuilder(module);
    di_file = di->createFile(filename, directory);
    di->createCompileUnit(llvm::dwarf::DW_LANG_C_plus_plus, di_file, "scpp", false, "", 0);
    module.addModuleFlag(llvm::Module::Warning, "Debug Info Version", llvm::DEBUG_METADATA_VERSION);
    module.addModuleFlag(llvm::Module::Warning, "Dwarf Version", llvm::dwarf::DWARF_VERSION);
}

void Codegen::finish_debug()
{
    if (di == nullptr)
        return;
    di->finalize();
    delete di;
    di = nullptr;
    di_file = nullptr;
    di_types.clear();
}

void Codegen::set_focus(const AstNode *node)
{
    focus = node;
    if (di == nullptr || function == nullptr || function->getSubprogram() == nullptr)
        return;
    unsigned line = function->getSubprogram()->getLine();
    unsigned column = 1;
    if (line == 0)
        line = 1;
    if (node != nullptr && node->line > 0) {
        line = static_cast<unsigned>(node->line);
        if (node->column > 0)
            column = static_cast<unsigned>(node->column);
    }
    builder.SetCurrentDebugLocation(
        llvm::DILocation::get(context, line, column, function->getSubprogram()));
}

std::string Codegen::debug_basic_name(const Ty &ty) const
{
    if (ty.is_bool)
        return "bool";
    if (ty.is_float)
        return ty.fp_rank >= 2 ? "double" : "float";
    if (ty.ir == nullptr || !ty.ir->isIntegerTy())
        return "int";
    std::string name;
    switch (ty.ir->getIntegerBitWidth()) {
    case 8:
        name = "char";
        break;
    case 16:
        name = "short";
        break;
    case 32:
        name = "int";
        break;
    default:
        name = "long";
        break;
    }
    if (!ty.is_signed)
        name = "unsigned " + name;
    return name;
}

llvm::DIType *Codegen::debug_record(const Ty &ty)
{
    auto cached = di_types.find(ty.record);
    if (cached != di_types.end())
        return cached->second;
    auto found = records.find(ty.record);
    const bool is_class = found != records.end() && found->second.is_class;
    const unsigned tag = ty.is_union ? llvm::dwarf::DW_TAG_union_type
        : (is_class ? llvm::dwarf::DW_TAG_class_type : llvm::dwarf::DW_TAG_structure_type);
    std::string name = debug_tag_name(found != records.end() ? source_name(found->second) : ty.record);
    llvm::DICompositeType *forward = di->createReplaceableCompositeType(tag, name, di_file, di_file, 0);
    di_types[ty.record] = forward;

    std::vector<llvm::Metadata *> elements;
    uint64_t size = 0;
    uint32_t align = 0;
    if (found != records.end() && found->second.ir != nullptr && !found->second.ir->isOpaque()) {
        const llvm::DataLayout &layout = module.getDataLayout();
        const llvm::StructLayout *slots = layout.getStructLayout(found->second.ir);
        size = layout.getTypeAllocSizeInBits(found->second.ir);
        align = static_cast<uint32_t>(layout.getABITypeAlign(found->second.ir).value() * 8);
        const unsigned count = found->second.ir->getNumElements();
        for (const Field &field : found->second.fields) {
            if (field.name.empty() || field.name[0] == '.' || field.index >= count)
                continue;
            llvm::DIType *member_type = debug_type(field.ty);
            if (member_type == nullptr)
                continue;
            llvm::Type *storage = field.ty.ir != nullptr ? field.ty.ir : llvm::Type::getInt32Ty(context);
            const uint64_t member_size = field.bits > 0
                ? static_cast<uint64_t>(field.bits)
                : layout.getTypeAllocSizeInBits(storage);
            elements.push_back(di->createMemberType(
                forward, field.name, di_file, 0, member_size, 0, slots->getElementOffsetInBits(field.index),
                llvm::DINode::FlagZero, member_type));
        }
    }
    llvm::DINodeArray members = di->getOrCreateArray(elements);
    llvm::DICompositeType *full = nullptr;
    if (tag == llvm::dwarf::DW_TAG_union_type)
        full = di->createUnionType(di_file, name, di_file, 0, size, align, llvm::DINode::FlagZero, members);
    else if (tag == llvm::dwarf::DW_TAG_class_type)
        full = di->createClassType(
            di_file, name, di_file, 0, size, align, 0, llvm::DINode::FlagZero, nullptr, members);
    else
        full = di->createStructType(
            di_file, name, di_file, 0, size, align, llvm::DINode::FlagZero, nullptr, members);
    llvm::TempMDNode owned(forward);
    llvm::DIType *done = di->replaceTemporary(std::move(owned), full);
    di_types[ty.record] = done;
    return done;
}

llvm::DIType *Codegen::debug_type(const Ty &ty)
{
    if (di == nullptr)
        return nullptr;
    if (ty.is_void || ty.ir == nullptr)
        return nullptr;
    std::string key = type_tag(ty);
    if (ty.is_const)
        key += " const";
    if (ty.is_volatile)
        key += " volatile";
    if (ty.is_enum)
        key = "enum " + ty.record;
    auto cached = di_types.find(key);
    if (cached != di_types.end())
        return cached->second;

    llvm::DIType *built = nullptr;
    if (ty.is_enum) {
        Ty underlying = ty;
        underlying.is_enum = false;
        underlying.record.clear();
        llvm::DIType *base = debug_type(underlying);
        const std::string name = debug_tag_name(ty.record);
        const llvm::DataLayout &layout = module.getDataLayout();
        built = di->createEnumerationType(
            di_file, name, di_file, 0, layout.getTypeAllocSizeInBits(ty.ir),
            static_cast<uint32_t>(layout.getABITypeAlign(ty.ir).value() * 8), di->getOrCreateArray({}), base);
    } else if (ty.is_pointer) {
        llvm::DIType *pointee = ty.inner != nullptr ? debug_type(*ty.inner) : nullptr;
        if (pointee == nullptr)
            pointee = di->createUnspecifiedType("void");
        built = di->createPointerType(pointee, module.getDataLayout().getPointerSizeInBits(0));
    } else if (ty.is_array && ty.inner != nullptr) {
        llvm::DIType *element = debug_type(*ty.inner);
        if (element == nullptr)
            element = di->createUnspecifiedType("void");
        const llvm::DataLayout &layout = module.getDataLayout();
        const uint64_t element_size = ty.inner->ir != nullptr ? layout.getTypeAllocSizeInBits(ty.inner->ir) : 0;
        const int64_t count = ty.array_len > 0 ? ty.array_len : 0;
        llvm::DISubrange *bounds = di->getOrCreateSubrange(0, count);
        const uint32_t align = ty.inner->ir != nullptr
            ? static_cast<uint32_t>(layout.getABITypeAlign(ty.inner->ir).value() * 8)
            : 0;
        built = di->createArrayType(
            element_size * static_cast<uint64_t>(count), align, element, di->getOrCreateArray({bounds}));
    } else if (ty.is_function) {
        std::vector<llvm::Metadata *> signature;
        llvm::DIType *ret = nullptr;
        if (ty.inner != nullptr && !ty.inner->is_void)
            ret = debug_type(*ty.inner);
        signature.push_back(ret);
        if (ty.sig != nullptr) {
            for (const Ty &param : ty.sig->params)
                signature.push_back(debug_type(param));
        }
        if (ty.func_vararg)
            signature.push_back(di->createUnspecifiedParameter());
        built = di->createSubroutineType(di->getOrCreateTypeArray(signature));
    } else if (ty.is_record || ty.is_union) {
        built = debug_record(ty);
    } else {
        const llvm::DataLayout &layout = module.getDataLayout();
        unsigned encoding = llvm::dwarf::DW_ATE_signed;
        if (ty.is_bool)
            encoding = llvm::dwarf::DW_ATE_boolean;
        else if (ty.is_float)
            encoding = llvm::dwarf::DW_ATE_float;
        else if (!ty.is_signed)
            encoding = llvm::dwarf::DW_ATE_unsigned;
        built = di->createBasicType(
            debug_basic_name(ty), layout.getTypeAllocSizeInBits(ty.ir), encoding);
    }
    if (built != nullptr && ty.is_const)
        built = di->createQualifiedType(llvm::dwarf::DW_TAG_const_type, built);
    if (built != nullptr && ty.is_volatile)
        built = di->createQualifiedType(llvm::dwarf::DW_TAG_volatile_type, built);
    if (built != nullptr)
        di_types[key] = built;
    return built;
}

void Codegen::begin_function_debug(Func &fn)
{
    if (di == nullptr || fn.fn == nullptr || fn.fn->getSubprogram() != nullptr)
        return;
    unsigned line = fn.line > 0 ? static_cast<unsigned>(fn.line) : 1;
    if (fn.body != nullptr && fn.line <= 0 && fn.body->line > 0)
        line = static_cast<unsigned>(fn.body->line);
    std::vector<llvm::Metadata *> signature;
    signature.push_back(fn.ret.is_void ? nullptr : debug_type(fn.ret));
    for (const Ty &param : fn.params)
        signature.push_back(debug_type(param));
    if (fn.vararg)
        signature.push_back(di->createUnspecifiedParameter());
    llvm::DISubroutineType *type = di->createSubroutineType(di->getOrCreateTypeArray(signature));
    const std::string &name = fn.debug_name.empty() ? fn.source : fn.debug_name;
    llvm::DISubprogram *subprogram = di->createFunction(
        di_file, name, fn.fn->getName(), di_file, line, type, line, llvm::DINode::FlagPrototyped,
        llvm::DISubprogram::SPFlagDefinition);
    fn.fn->setSubprogram(subprogram);
    builder.SetCurrentDebugLocation(llvm::DILocation::get(context, line, 1, subprogram));
}

void Codegen::declare_param(llvm::AllocaInst *addr, const std::string &name, const Ty &ty, unsigned arg_no,
                            unsigned line)
{
    if (di == nullptr || addr == nullptr || function == nullptr || function->getSubprogram() == nullptr)
        return;
    if (name.empty() || name[0] == '.')
        return;
    llvm::DIType *type = debug_type(ty);
    if (type == nullptr)
        return;
    if (line == 0)
        line = function->getSubprogram()->getLine();
    llvm::DILocalVariable *variable = di->createParameterVariable(
        function->getSubprogram(), name, arg_no, di_file, line, type, true);
    llvm::DILocation *at = llvm::DILocation::get(context, line == 0 ? 1 : line, 1, function->getSubprogram());
    di->insertDeclare(addr, variable, di->createExpression(), at, addr->getParent());
}

void Codegen::declare_local(llvm::AllocaInst *addr, const std::string &name, const Ty &ty)
{
    if (di == nullptr || addr == nullptr || function == nullptr || function->getSubprogram() == nullptr)
        return;
    if (name.empty() || name[0] == '.')
        return;
    llvm::DIType *type = debug_type(ty);
    if (type == nullptr)
        return;
    unsigned line = focus != nullptr && focus->line > 0 ? static_cast<unsigned>(focus->line)
                                                        : function->getSubprogram()->getLine();
    if (line == 0)
        line = 1;
    llvm::DILocalVariable *variable = di->createAutoVariable(
        function->getSubprogram(), name, di_file, line, type, true);
    llvm::DILocation *at = llvm::DILocation::get(context, line, 1, function->getSubprogram());
    llvm::BasicBlock *block = builder.GetInsertBlock() != nullptr ? builder.GetInsertBlock() : addr->getParent();
    di->insertDeclare(addr, variable, di->createExpression(), at, block);
}
