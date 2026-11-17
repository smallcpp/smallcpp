/*
 * Types, scalars, expressions, and control flow.
 */
#include "lower.hpp"

extern "C" {
#include "diagnostic.h"
}

void Codegen::error(const char *message)
{
    if (!failed) {
        int line = focus != NULL && focus->line > 0 ? focus->line : 1;
        int column = focus != NULL && focus->column > 0 ? focus->column : 1;
        smallcpp_diag_report(line, column, message);
    }
    failed = true;
}

void Codegen::error_name(const char *message, const char *name)
{
    if (!failed) {
        std::string text = std::string(message) + ": " + (name != NULL ? name : "");
        int line = focus != NULL && focus->line > 0 ? focus->line : 1;
        int column = focus != NULL && focus->column > 0 ? focus->column : 1;
        smallcpp_diag_report(line, column, text.c_str());
    }
    failed = true;
}

void Codegen::note_storage(Storage *storage, bool already, bool *flag, const char *name)
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

Ty Codegen::drop(Ty ty)
{
    ty.is_const = false;
    ty.is_volatile = false;
    return ty;
}

Ty Codegen::integer(unsigned bits, bool is_signed, int rank)
{
    Ty ty;
    ty.ir = llvm::Type::getIntNTy(context, bits);
    ty.is_signed = is_signed;
    ty.is_bool = bits == 1;
    ty.rank = rank;
    return ty;
}

Ty Codegen::floating(bool is_double)
{
    Ty ty;
    ty.ir = is_double ? llvm::Type::getDoubleTy(context) : llvm::Type::getFloatTy(context);
    ty.is_float = true;
    ty.fp_rank = is_double ? 2 : 1;
    return ty;
}

Ty Codegen::bool_ty()
{
    return integer(1, false, 0);
}

Ty Codegen::int_ty()
{
    return integer(32, true, 3);
}

Ty Codegen::size_ty()
{
    return integer(64, false, 4);
}

Ty Codegen::void_ty()
{
    Ty ty;
    ty.ir = llvm::Type::getVoidTy(context);
    ty.is_void = true;
    return ty;
}

RValue Codegen::fallback()
{
    RValue value;
    value.ty = int_ty();
    value.v = llvm::ConstantInt::get(value.ty.ir, 0);
    return value;
}

bool Codegen::op_is(const AstNode *node, const char *text)
{
    return node != NULL && node->text != NULL && std::strcmp(node->text, text) == 0;
}

bool Codegen::is_integer(const Ty &ty)
{
    return !ty.is_void && !ty.is_float && ty.ir != NULL && ty.ir->isIntegerTy();
}

void Codegen::reject_node(const AstNode *node)
{
    SourceNote here(*this, node);
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

void Codegen::reject_declarator(const AstNode *node)
{
    SourceNote here(*this, node);
    if (node == NULL)
        error("declaration has no name");
    else if (node->kind == AST_QUALIFIED)
        error("qualified names are a later phase");
    else
        error("not a declarator");
}

Ty Codegen::pointer_to(Ty inner)
{
    Ty ty;
    ty.ir = llvm::PointerType::get(context, 0);
    ty.is_pointer = true;
    ty.inner = std::make_shared<Ty>(inner);
    return ty;
}

Ty Codegen::array_of(Ty elem, int length)
{
    Ty ty;
    ty.ir = llvm::ArrayType::get(elem.ir, static_cast<uint64_t>(length));
    ty.is_array = true;
    ty.array_len = length;
    ty.inner = std::make_shared<Ty>(elem);
    return ty;
}

Ty Codegen::decay(Ty ty)
{
    ty = drop(ty);
    if (ty.is_array && ty.inner)
        return pointer_to(*ty.inner);
    return ty;
}

bool Codegen::complete_type(const Ty &ty)
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

unsigned Codegen::abi_align(llvm::Type *ty)
{
    if (ty == NULL)
        return 1;
    return static_cast<unsigned>(module.getDataLayout().getABITypeAlign(ty).value());
}

void Codegen::note_quals(Ty &ty, const AstNode *list)
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

Ty Codegen::apply_pointers(Ty base, const AstNode *node)
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

bool Codegen::declarator_form(const AstNode *node)
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

int Codegen::const_i64(const AstNode *node, int64_t &out)
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

Ty Codegen::walk_declarator(Ty base, const AstNode *node, Declared &out)
{
    SourceNote here(*this, node);
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

Declared Codegen::declare_one(Ty base, const AstNode *node)
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

Ty Codegen::parse_specs(const AstNode *list, Where where, Storage *storage)
{
    SourceNote here(*this, list);
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
            if (base != Missing || shorts || longs || saw_signed || saw_unsigned || saw_int)
                error("invalid type");
            base = Alias;
            if (node->kind == AST_CLASS)
                alias_ty = make_class(node);
            else
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
            note_storage(storage, storage == NULL ? false : storage->is_virtual,
                         storage == NULL ? nullptr : &storage->is_virtual, "virtual");
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

bool Codegen::is_type_form(const AstNode *node)
{
    if (node == NULL)
        return false;
    return node->kind == AST_SPECIFIER || node->kind == AST_DECLARATOR
        || node->kind == AST_STRUCT || node->kind == AST_UNION
        || node->kind == AST_ENUM || node->kind == AST_CLASS
        || node->kind == AST_TYPE_NAME;
}

Ty Codegen::type_from_name(const AstNode *node)
{
    SourceNote here(*this, node);
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

Ty Codegen::promote(Ty ty)
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

Ty Codegen::usual(Ty left, Ty right)
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

bool Codegen::suffix_is(const std::string &spell, const char *suffix)
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

bool Codegen::decode_char(const char *text, unsigned &code)
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

bool Codegen::looks_float(const char *text)
{
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        return false;
    if (std::strchr(text, '.') != NULL || std::strpbrk(text, "eE") != NULL)
        return true;
    std::size_t length = std::strlen(text);
    return length != 0 && (text[length - 1] == 'f' || text[length - 1] == 'F');
}

bool Codegen::decode_literal(const char *text, Ty &ty, llvm::Constant *&value)
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

llvm::Constant *Codegen::convert_const(llvm::Constant *value, const Ty &from, const Ty &to)
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

llvm::Value *Codegen::convert(llvm::Value *value, const Ty &from, const Ty &to)
{
    if (failed)
        return llvm::Constant::getNullValue(to.ir != NULL ? to.ir : llvm::Type::getInt32Ty(context));
    if (from.is_pointer || to.is_pointer) {
        if (from.is_pointer && to.is_pointer) {
            if (from.inner != NULL && to.inner != NULL && class_type(*from.inner) && class_type(*to.inner)
                && from.inner->record != to.inner->record
                && derives_from(from.inner->record, to.inner->record))
                return adjust_to(value, from.inner->record, to.inner->record);
            return value;
        }
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

RValue Codegen::cast_to(RValue value, Ty dest)
{
    if (failed)
        return fallback();
    RValue out;
    out.ty = drop(dest);
    out.v = convert(value.v, value.ty, dest);
    return out;
}

void Codegen::push_scope()
{
    scopes.emplace_back();
    blocks.emplace_back();
}

void Codegen::pop_scope()
{
    if (scopes.size() > 1)
        scopes.pop_back();
    if (!blocks.empty())
        blocks.pop_back();
}

void Codegen::bind(const std::string &name, Slot slot)
{
    if (name.empty())
        return;
    if (scopes.back().count(name) != 0 || enum_consts.count(name) != 0) {
        error_name("name is already defined", name.c_str());
        return;
    }
    scopes.back()[name] = slot;
}

llvm::AllocaInst *Codegen::alloca_entry(llvm::Type *ty, const std::string &name)
{
    llvm::IRBuilder<> allocas(entry, entry->begin());
    return allocas.CreateAlloca(ty, nullptr, name);
}

llvm::Value *Codegen::load_slot(const Slot &slot)
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

void Codegen::store_slot(const Slot &slot, llvm::Value *value, bool initializing)
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

RValue Codegen::read_slot(const Slot &slot)
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

Ty Codegen::object_type(const AstNode *node)
{
    SourceNote here(*this, node);
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
            std::optional<Ty> member = implicit_member_type(node->text);
            if (member)
                return *member;
            if (failed)
                return int_ty();
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
        std::optional<Ty> member = member_type(base, node->text);
        if (!member) {
            if (!failed)
                error_name("no such member", node->text);
            return int_ty();
        }
        return *member;
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

std::optional<Slot> Codegen::index_slot(const AstNode *node)
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

bool Codegen::terminated()
{
    llvm::BasicBlock *block = builder.GetInsertBlock();
    return block == NULL || block->getTerminator() != NULL;
}

void Codegen::ensure_insert()
{
    if (failed)
        return;
    if (!terminated())
        return;
    llvm::BasicBlock *dead = llvm::BasicBlock::Create(context, "dead", function);
    builder.SetInsertPoint(dead);
}

llvm::BasicBlock *Codegen::new_block(const char *name)
{
    return llvm::BasicBlock::Create(context, name, function);
}

std::optional<Slot> Codegen::lvalue(const AstNode *node)
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
            slot = implicit_member(node->text);
        if (!slot && !failed)
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
        return member_slot(base->addr, base->ty, node->text, true);
    }
    if (node->kind == AST_ARROW) {
        RValue ptr = emit_rvalue(node->a);
        if (failed)
            return std::nullopt;
        if (!ptr.ty.is_pointer || ptr.ty.inner == NULL) {
            error("not a pointer");
            return std::nullopt;
        }
        return member_slot(ptr.v, *ptr.ty.inner, node->text, true);
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

Ty Codegen::type_of(const AstNode *node)
{
    SourceNote here(*this, node);
    if (failed || node == NULL)
        return int_ty();
    if (node->kind == AST_THIS) {
        if (method_class.empty() || this_ty.ir == NULL) {
            error("this is not allowed here");
            return int_ty();
        }
        return this_ty;
    }
    if (node->kind == AST_NULL)
        return null_ty();
    if (node->kind == AST_DELETE) {
        type_of(node->a);
        return void_ty();
    }
    if (node->kind == AST_NEW) {
        std::vector<const AstNode *> bounds;
        const AstNode *allocated = node->a;
        if (allocated != NULL && allocated->kind == AST_POINTER)
            allocated = allocated->a;
        Ty elem = new_element(allocated, bounds);
        if (elem.is_array && elem.inner != NULL)
            elem = *elem.inner;
        return pointer_to(elem);
    }
    if (node->kind == AST_PRINT || node->kind == AST_FORMAT) {
        for (const AstNode *arg = node->a; arg != NULL && !failed; arg = arg->next)
            type_of(arg);
        return node->kind == AST_FORMAT ? string_ty() : void_ty();
    }
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
            std::optional<Ty> member = implicit_member_type(node->text);
            if (member)
                return drop(*member);
            if (failed)
                return int_ty();
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

Ty Codegen::type_unary(const AstNode *node)
{
    SourceNote here(*this, node);
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

Ty Codegen::type_binary(const AstNode *node)
{
    SourceNote here(*this, node);
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

Const Codegen::eval_const(const AstNode *node)
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

Const Codegen::eval_sizeof(const AstNode *operand)
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

Const Codegen::with_const(llvm::Constant *value, Ty ty)
{
    Const out;
    out.ok = true;
    out.v = value;
    out.ty = ty;
    return out;
}

Const Codegen::eval_unary(const AstNode *node)
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

bool Codegen::const_true(const Const &value)
{
    if (value.ty.is_float)
        return !llvm::cast<llvm::ConstantFP>(value.v)->isZero();
    return !llvm::cast<llvm::ConstantInt>(value.v)->isZero();
}

Const Codegen::eval_binary(const AstNode *node)
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

Const Codegen::fold_int(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare)
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

Const Codegen::fold_float(const char *op, llvm::Constant *lhs, llvm::Constant *rhs, Ty dest, bool compare)
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

Const Codegen::eval_conditional(const AstNode *node)
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

RValue Codegen::emit_rvalue(const AstNode *node)
{
    SourceNote here(*this, node);
    if (failed || node == NULL)
        return fallback();
    if (node->kind == AST_NULL) {
        RValue out;
        out.ty = null_ty();
        out.v = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
        return out;
    }
    if (node->kind == AST_NEW)
        return emit_new(node);
    if (node->kind == AST_DELETE) {
        emit_delete(node);
        RValue out;
        out.ty = void_ty();
        return out;
    }
    if (node->kind == AST_PRINT || node->kind == AST_FORMAT)
        return emit_native(node);
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
    if (node->kind == AST_THIS) {
        if (method_class.empty()) {
            error("this is not allowed here");
            return fallback();
        }
        std::optional<Slot> slot = lookup("this");
        if (!slot) {
            error("this is not allowed here");
            return fallback();
        }
        return read_slot(*slot);
    }
    if (node->kind == AST_IDENT) {
        std::optional<Slot> slot = lookup(node->text);
        if (!slot)
            slot = implicit_member(node->text);
        if (!slot) {
            if (failed)
                return fallback();
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

RValue Codegen::emit_cast(const AstNode *node)
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

RValue Codegen::emit_unary(const AstNode *node)
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

RValue Codegen::emit_inc(const AstNode *node)
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

llvm::Value *Codegen::emit_bool(const AstNode *node)
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

RValue Codegen::emit_and(const AstNode *left, const AstNode *right)
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

RValue Codegen::emit_or(const AstNode *left, const AstNode *right)
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

RValue Codegen::emit_binary(const AstNode *node)
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

RValue Codegen::apply_op(const char *op, RValue left, RValue right)
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

RValue Codegen::emit_assign(const AstNode *node)
{
    SourceNote here(*this, node);
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

RValue Codegen::emit_conditional(const AstNode *node)
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

void Codegen::emit_statement(const AstNode *node)
{
    SourceNote here(*this, node);
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
    case AST_TRY:
        emit_try(node);
        break;
    case AST_THROW:
        emit_throw(node);
        break;
    case AST_USING:
        error("namespaces are a later phase");
        break;
    default:
        reject_node(node);
        break;
    }
}

void Codegen::emit_items(const AstNode *item)
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

void Codegen::emit_compound(const AstNode *node)
{
    push_scope();
    emit_items(node->a);
    pop_scope();
}

void Codegen::emit_if(const AstNode *node)
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

bool Codegen::case_key(const AstNode *expr, const Ty &dest, uint64_t &key, llvm::ConstantInt *&iconst)
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

void Codegen::collect_cases(const AstNode *node)
{
    for (const AstNode *item = node; item != NULL && !failed; item = item->next)
        collect_one(item);
}

void Codegen::collect_one(const AstNode *node)
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

void Codegen::emit_switch(const AstNode *node)
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

void Codegen::emit_case(const AstNode *node)
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

void Codegen::emit_default(const AstNode *node)
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

void Codegen::emit_while(const AstNode *node)
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

void Codegen::emit_do(const AstNode *node)
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

void Codegen::emit_for(const AstNode *node)
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

void Codegen::emit_label(const AstNode *node)
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

void Codegen::emit_goto(const AstNode *node)
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

void Codegen::emit_break()
{
    if (loops.empty() || loops.back().break_bb == NULL) {
        error("break is outside a loop or switch");
        return;
    }
    builder.CreateBr(loops.back().break_bb);
}

void Codegen::emit_continue()
{
    for (auto frame = loops.rbegin(); frame != loops.rend(); ++frame) {
        if (frame->continue_bb == NULL)
            continue;
        builder.CreateBr(frame->continue_bb);
        return;
    }
    error("continue is outside a loop");
}

void Codegen::emit_return(const AstNode *node)
{
    if (ret_ty.is_void) {
        if (node->a != NULL) {
            RValue value = emit_rvalue(node->a);
            if (!failed && !value.ty.is_void)
                error("void function returns a value");
        }
        if (!failed && !terminated()) {
            close_open_catches();
            builder.CreateRetVoid();
        }
        return;
    }
    if (node->a == NULL) {
        error("return needs a value");
        return;
    }
    RValue value = cast_to(emit_rvalue(node->a), ret_ty);
    if (!failed && !terminated()) {
        close_open_catches();
        builder.CreateRet(value.v);
    }
}

const AstNode *Codegen::scalar_declarator(const AstNode *node, const AstNode *&init)
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

void Codegen::place_value(const std::string &name, Ty ty, llvm::Value *addr)
{
    if (ty.is_union) {
        if (auto *global = llvm::dyn_cast<llvm::GlobalVariable>(addr))
            global->setAlignment(llvm::Align(record_align(ty)));
        else if (auto *alloca = llvm::dyn_cast<llvm::AllocaInst>(addr))
            alloca->setAlignment(llvm::Align(record_align(ty)));
    }
    bind(name, Slot{addr, ty});
}

void Codegen::emit_declaration(const AstNode *node, bool file)
{
    SourceNote here(*this, node);
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
            declare_local(addr, declared.name->text, object);
            if (declared.init != NULL)
                store_initializer(slot, declared.init);
        }
    }
}

void Codegen::emit_body(Func &fn)
{
    scopes.resize(1);
    blocks.clear();
    labels.clear();
    loops.clear();
    switches.clear();
    handlers.clear();
    in_handler = 0;
    catch_open.clear();
    std::vector<std::string> saved_ns = ns_stack;
    const std::vector<LookupScope> *saved_lookup = active_lookup;
    std::string saved_class = method_class;
    bool saved_const = method_const;
    Ty saved_this = this_ty;
    ns_stack = fn.home;
    active_lookup = &fn.lookup;
    method_class = fn.klass;
    method_const = fn.method_const;
    function = fn.fn;
    ret_ty = fn.ret;
    entry = llvm::BasicBlock::Create(context, "entry", function);
    begin_function_debug(fn);
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
        declare_param(addr, name, fn.params[i], static_cast<unsigned>(i + 1),
                      fn.line > 0 ? static_cast<unsigned>(fn.line) : 1);
        store_slot(slot, &*argument, true);
        ++argument;
    }
    if (!fn.klass.empty()) {
        std::optional<Slot> self = lookup("this");
        if (self)
            this_ty = self->ty;
    }
    if (fn.is_ctor)
        emit_inits(fn);
    if (fn.is_dtor) {
        std::optional<Slot> self = lookup("this");
        if (self && self->addr != NULL && self->ty.ir != NULL) {
            llvm::Value *pointer = builder.CreateLoad(self->ty.ir, self->addr);
            llvm::Function *done = dtor_done_fn();
            builder.CreateCall(done->getFunctionType(), done, {pointer});
        }
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
        if (di != nullptr)
            set_focus(fn.body);
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
    builder.SetCurrentDebugLocation(llvm::DebugLoc());
    pop_scope();
    scopes.resize(1);
    blocks.clear();
    ns_stack = saved_ns;
    active_lookup = saved_lookup;
    method_class = saved_class;
    method_const = saved_const;
    this_ty = saved_this;
    function = nullptr;
    entry = nullptr;
}
