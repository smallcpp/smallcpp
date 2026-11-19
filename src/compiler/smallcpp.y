/*
 * smallcpp grammar. Start symbol: translation_unit.
 *
 * The base language is the syntax shared by ISO C99 and ISO C++11, so a
 * program that stays in that subset is acceptable to a modern C compiler
 * and a modern C++ compiler. smallcpp adds classes, inheritance,
 * exceptions, namespaces, and precise garbage collection. A class may
 * list more than one base. A base may be virtual, so two paths share one
 * subobject. A virtual method may be pure, written = 0. A destructor
 * may be virtual. typeid(expression) and typeid(type) yield a const
 * TypeInfo pointer. == and != compare that pointer. A polymorphic
 * object uses the dynamic type. catch (...) catches every exception type.
 * That ... is not a parameter. A constructor is a method whose name is
 * the class name and that has no return type. A destructor is
 * ~ClassName() and may take parameters. A constructor may have an
 * initializer list: ClassName(args) : member(expr), base(expr) { ... }.
 * A parameter may have a default, written name = expression. A
 * trailing destructor parameter may have one too. using name;
 * and using namespace name; are supported. Anonymous namespaces are not.
 * Two functions may share a name when their parameter lists differ.
 * Operator overloading is not supported. super is not a keyword. A base
 * method is Base::method(), or method() when that name is not hidden.
 * this remains a keyword.
 * A method may be const, written after its parameter list. () is an
 * empty parameter list and means no parameters. bool, true, and false
 * are keywords. A new-expression may allocate an array or a pointer.
 * Struct and union members may be bit-fields. An initializer may use a
 * designator or a brace list. A compound literal, (type){ ... }, does
 * not parse. smallcpp does not add templates, lambdas, references,
 * ref-qualifiers, operator overloading, or the C++ standard library.
 * print and format
 * are native, not declarations the
 * program supplies. print writes formatted text. format produces the
 * formatted string. Each takes a string literal in C++23 format-string
 * syntax, then the arguments that fill its replacement fields.
 *
 * No keyword has an sc_ prefix. class, namespace, public, private,
 * protected, virtual, try, catch, throw, new, this, print, format, bool,
 * true, false, and typeid are the plain spellings, not macros. namespace uses
 * C++ syntax:
 * namespace name { ... }.
 * A qualified name uses '::'. A new-expression creates a pointer. delete
 * type-checks its pointer and calls the destructor. It does not deallocate.
 * A direct call is object.~Class(args) or pointer->~Class(args).
 * The argument list may be empty. nullptr and NULL are keywords for
 * the same null pointer constant.
 * Source files use the .scpp extension. Ordinary preprocessing is already
 * done. Semantic actions build a syntax tree. They do not generate code.
 *
 * A syntax error prints the file, line, and column, the source line, and
 * the tokens that were expected. There is no caret under the line. The
 * error token then resynchronizes at the next semicolon so later errors
 * are still reported. That recovery is not part of the language.
 *
 * Deliberate omissions:
 *   - variable-length arrays, including '[*]' and 'static' inside '[]'
 *   - restrict
 *   - flexible array members. Empty '[]' is recognized only on parameters,
 *     where both languages adjust the parameter to a pointer
 *   - compound literals. (type){ ... } does not parse, including (int[]){1, 2}
 *   - C++11 brace initialization without '='. A brace list is written
 *     after '='. references, rvalue references, auto as a type, lambdas,
 *     constexpr, trailing return types, and ref-qualifiers. auto is only
 *     the C storage-class specifier
 *   - _Bool, _Complex, and _Imaginary, which are not C++11. wchar_t is
 *     not a keyword
 *   - K&R definitions that name parameters before the body. () has no
 *     parameters
 *   - empty structs, anonymous struct members, anonymous union members,
 *     and unnamed bit-fields. A bit-field names its member
 *   - u, U, and u8 string prefixes, which are not C99
 *   - anonymous namespaces. A namespace has a name
 *   - super. A base method is Base::method(), or method() when the name
 *     is not hidden. this remains a keyword
 *   - templates and operator overloading
 *   - an array bound is syntactically an expression. A bound that is not an
 *     integer constant expression is a VLA and is excluded
 */

%{
#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int yylex(void);
void yyerror(const char *message);

static int count_args(const struct AstNode *arg)
{
    int count = 0;

    for (; arg != NULL; arg = arg->next)
        count++;
    return count;
}

static struct AstNode *native_format(AstKind kind, char *literal, struct AstNode *args)
{
    char message[160];

    if (!ast_format_ok(literal, count_args(args), message, sizeof message))
        yyerror(message);
    return ast_make(kind, literal, args, NULL, NULL, NULL);
}

#define NAME_MAX 256

static char *pointer_names[NAME_MAX];
static int pointer_count;

static void remember_pointer(const char *name)
{
    int i;

    if (name == NULL || pointer_count >= NAME_MAX)
        return;
    for (i = 0; i < pointer_count; i++) {
        if (strcmp(pointer_names[i], name) == 0)
            return;
    }
    pointer_names[pointer_count] = strdup(name);
    if (pointer_names[pointer_count] != NULL)
        pointer_count++;
}

static int name_is_pointer(const char *name)
{
    int i;

    if (name == NULL)
        return 0;
    for (i = 0; i < pointer_count; i++) {
        if (strcmp(pointer_names[i], name) == 0)
            return 1;
    }
    return 0;
}

/* The language server parses more than one buffer in one process. */
void smallcpp_reset_parser(void)
{
    int i;

    for (i = 0; i < pointer_count; i++)
        free(pointer_names[i]);
    pointer_count = 0;
}

static const char *declarator_name(const struct AstNode *node)
{
    const char *name;

    if (node == NULL)
        return NULL;
    if (node->kind == AST_IDENT || node->kind == AST_TYPE_NAME)
        return node->text;
    name = declarator_name(node->a);
    if (name != NULL)
        return name;
    return declarator_name(node->b);
}

static int declarator_is_pointer(const struct AstNode *node)
{
    if (node == NULL)
        return 0;
    if (node->kind == AST_INIT)
        return declarator_is_pointer(node->a);
    if (node->kind == AST_POINTER)
        return 1;
    if (node->kind == AST_DECLARATOR)
        return declarator_is_pointer(node->a) || declarator_is_pointer(node->b);
    return 0;
}

static void note_declarator(const struct AstNode *node)
{
    if (node == NULL)
        return;
    if (node->kind == AST_INIT)
        node = node->a;
    if (declarator_is_pointer(node))
        remember_pointer(declarator_name(node));
}

static void note_declarators(const struct AstNode *node)
{
    for (; node != NULL; node = node->next)
        note_declarator(node);
}

static int type_is_pointer(const struct AstNode *type)
{
    if (type == NULL)
        return 0;
    if (type->kind == AST_POINTER)
        return 1;
    if (type->kind == AST_DECLARATOR)
        return type_is_pointer(type->a) || type_is_pointer(type->b);
    return 0;
}

static int operand_is_pointer(const struct AstNode *node)
{
    if (node == NULL)
        return 0;
    switch (node->kind) {
    case AST_NULL:
    case AST_NEW:
        return 1;
    case AST_UNARY:
        return node->text != NULL && strcmp(node->text, "&") == 0;
    case AST_IDENT:
    case AST_TYPE_NAME:
        return name_is_pointer(node->text);
    case AST_CAST:
        return type_is_pointer(node->a);
    case AST_BINARY:
        if (node->text != NULL
            && (strcmp(node->text, "+") == 0 || strcmp(node->text, "-") == 0))
            return operand_is_pointer(node->a) || operand_is_pointer(node->b);
        return 0;
    case AST_CONDITIONAL:
        return operand_is_pointer(node->b) && operand_is_pointer(node->c);
    default:
        return 0;
    }
}

void smallcpp_push_class(const char *name);
void smallcpp_pop_class(void);
const char *smallcpp_current_class(void);

static int text_is(const struct AstNode *name, const char *text)
{
    return name != NULL && name->text != NULL && text != NULL
        && strcmp(name->text, text) == 0;
}

static int names_equal(const struct AstNode *a, const struct AstNode *b)
{
    return a != NULL && b != NULL && a->text != NULL && b->text != NULL
        && strcmp(a->text, b->text) == 0;
}

static char *steal_text(struct AstNode *name)
{
    char *spelling = NULL;

    if (name != NULL) {
        spelling = name->text;
        name->text = NULL;
        ast_free(name);
    }
    return spelling;
}

static struct AstNode *make_ctor(struct AstNode *name, struct AstNode *params,
                                 struct AstNode *inits, struct AstNode *body)
{
    return ast_make(AST_CONSTRUCTOR, steal_text(name), params, inits, body, NULL);
}

static struct AstNode *ctor_spelling(char *spelling, struct AstNode *params,
                                     struct AstNode *inits, struct AstNode *body)
{
    return ast_make(AST_CONSTRUCTOR, spelling, params, inits, body, NULL);
}

static struct AstNode *make_dtor(struct AstNode *name, struct AstNode *params,
                                 struct AstNode *body, int is_virtual)
{
    return ast_make(AST_DESTRUCTOR, steal_text(name), body,
                    is_virtual ? ast_named(AST_SPECIFIER, "virtual") : NULL,
                    params, NULL);
}

static struct AstNode *dtor_call(struct AstNode *object, struct AstNode *name,
                                 struct AstNode *args, int arrow)
{
    char *spelling = steal_text(name);
    char *label = NULL;
    size_t length = spelling == NULL ? 0 : strlen(spelling);

    if (spelling != NULL) {
        label = malloc(length + 2);
        if (label != NULL) {
            label[0] = '~';
            memcpy(label + 1, spelling, length + 1);
        }
        free(spelling);
    }
    return ast_make(AST_CALL, NULL,
                    ast_make(arrow ? AST_ARROW : AST_MEMBER, label, object, NULL, NULL, NULL),
                    args, NULL, NULL);
}

static struct AstNode *make_mem_init(struct AstNode *name, struct AstNode *args)
{
    return ast_make(AST_MEM_INIT, steal_text(name), args, NULL, NULL, NULL);
}

/* The innermost pointer or array has no element type yet. */
static struct AstNode *graft(struct AstNode *outer, struct AstNode *inner)
{
    struct AstNode *node = outer;

    if (outer == NULL)
        return inner;
    while (node->a != NULL)
        node = node->a;
    node->a = inner;
    return outer;
}

static struct AstNode *make_new(struct AstNode *base, struct AstNode *pointer,
                                struct AstNode *array, struct AstNode *args)
{
    struct AstNode *type = base;

    if (pointer != NULL)
        type = graft(pointer, type);
    if (array != NULL)
        type = graft(array, type);
    return ast_new_expr(type, args);
}

static struct AstNode *designate(struct AstNode *designators, struct AstNode *value)
{
    struct AstNode *last = designators;

    if (last == NULL)
        return value;
    while (last->next != NULL)
        last = last->next;
    last->b = value;
    return designators;
}
%}

%locations
%define parse.error custom

%union {
    char *text;
    struct AstNode *node;
    int kind;
}

%token <text> IDENTIFIER TYPEDEF_NAME CONSTANT STRING_LITERAL CTOR_NAME
%token CLASS "class" NAMESPACE "namespace"
%token PUBLIC "public" PRIVATE "private" PROTECTED "protected"
%token VIRTUAL "virtual" THIS "this"
%token TRY "try" CATCH "catch" THROW "throw" NEW "new"
%token PRINT "print" FORMAT "format"
%token NULLPTR "nullptr" NULL_KEYWORD "NULL" DELETE "delete"
%token USING "using"
%token BOOL "bool" TRUE "true" FALSE "false"
%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE ENUM
%token EXTERN FLOAT FOR GOTO IF INLINE INT LONG REGISTER RETURN SHORT
%token SIGNED SIZEOF STATIC STRUCT SWITCH TYPEDEF UNION UNSIGNED VOID
%token TYPEID "typeid"
%token VOLATILE WHILE
%token SCOPE "::"
%token ARROW "->" INC "++" DEC "--"
%token EQEQ "==" NOTEQ "!=" LE "<=" GE ">="
%token ANDAND "&&" OROR "||" SHL "<<" SHR ">>"
%token ADD_ASSIGN "+=" SUB_ASSIGN "-=" MUL_ASSIGN "*=" DIV_ASSIGN "/="
%token MOD_ASSIGN "%=" AND_ASSIGN "&=" OR_ASSIGN "|=" XOR_ASSIGN "^="
%token SHL_ASSIGN "<<=" SHR_ASSIGN ">>="
%token ELLIPSIS "..."

/* Dangling else binds to the nearest if, as in both C99 and C++11. */
%precedence LOWER_THAN_ELSE
%precedence ELSE
/* A function's = 0 is a pure specifier, not an initializer. */
%precedence LOWER_THAN_PURE
%precedence '='

/*
 * new int * p is a pointer type, and new int[n] is an array. The short
 * new-expression yields to '*' and '['. (new int) * p multiplies, and
 * (new int)[0] subscripts.
 */
%precedence NEW_BARE
%nonassoc '*'

%expect 0

%type <node> translation_unit external_declaration namespace_definition
%type <node> external_declaration_list function_definition declaration
%type <node> declaration_specifiers declaration_specifier storage_class_specifier
%type <node> type_specifier struct_or_union_specifier struct_declaration_list
%type <node> struct_declaration specifier_qualifier_list specifier_qualifier
%type <node> struct_declarator_list struct_declarator enum_specifier
%type <node> enumerator_list enumerator class_specifier name base_clause
%type <node> access_keyword class_member_list_opt class_member_list class_member
%type <node> type_qualifier function_specifier init_declarator_list
%type <node> init_declarator initializer initializer_list declarator
%type <node> direct_declarator declarator_id pointer type_qualifier_list
%type <node> parameter_type_list parameter_list parameter_declaration
%type <node> parameter_specifiers parameter_specifier parameter_declarator
%type <node> direct_parameter_declarator type_name abstract_declarator
%type <node> direct_abstract_declarator statement labeled_statement
%type <node> compound_statement block_item_list block_item expression_statement
%type <node> selection_statement iteration_statement jump_statement
%type <node> try_statement catch_clause_list catch_clause throw_statement
%type <node> base_specifier base_specifier_list constructor_member destructor_member
%type <node> destructor_params
%type <node> pure_specifier_opt
%type <node> constructor_definition destructor_definition ctor_initializer_opt
%type <node> mem_initializer_list mem_initializer using_declaration using_name
%type <node> primary_expression nested_name postfix_expression format_expression
%type <node> argument_expression_list unary_expression new_type cast_expression
%type <node> new_pointer new_arrays brace_initializer cv_qualifier_opt
%type <node> cv_qualifier_seq designator designator_list
%type <node> multiplicative_expression additive_expression shift_expression
%type <node> relational_expression equality_expression and_expression
%type <node> exclusive_or_expression inclusive_or_expression
%type <node> logical_and_expression logical_or_expression conditional_expression
%type <node> assignment_expression expression expression_opt constant_expression
%type <text> unary_operator assignment_operator
%type <kind> struct_or_union

%start translation_unit

%%

/* Start symbol. A translation unit is zero or more external declarations. */
translation_unit
    : %empty {
        $$ = ast_place(ast_make(AST_TRANSLATION_UNIT, NULL, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column);
        smallcpp_ast = $$;
      }
    | translation_unit external_declaration {
        ast_add($1, $2);
        $$ = ast_place($1, @$.first_line, @$.first_column);
        smallcpp_ast = $$;
      }
    ;

external_declaration
    : function_definition { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | constructor_definition { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | destructor_definition { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | namespace_definition { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | using_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | error ';' { yyerrok; $$ = NULL; }
    ;

/*
 * namespace name { ... }
 * The same syntax as C++. Nested namespaces are allowed.
 * A namespace has a name. Anonymous namespaces are not supported.
 * A namespace is not a statement and cannot appear inside a function.
 *
 * Two functions may share a name when their parameter lists differ.
 * That is overloading. operator+ and other operator functions are not
 * supported.
 */
namespace_definition
    : NAMESPACE IDENTIFIER '{' '}' {
        $$ = ast_place(ast_make(AST_NAMESPACE, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | NAMESPACE IDENTIFIER '{' external_declaration_list '}' {
        $$ = ast_place(ast_make(AST_NAMESPACE, $2, $4, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

external_declaration_list
    : external_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | external_declaration_list external_declaration { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

function_definition
    : declaration_specifiers declarator compound_statement {
        $$ = ast_place(ast_make(AST_METHOD, NULL, $1, $2, $3, NULL), @$.first_line, @$.first_column);
      }
    ;

declaration
    : declaration_specifiers init_declarator_list ';' {
        note_declarators($2);
        $$ = ast_place(ast_make(AST_DECLARATION, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    | declaration_specifiers ';' {
        $$ = ast_place(ast_make(AST_DECLARATION, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

declaration_specifiers
    : declaration_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | declaration_specifiers declaration_specifier { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

declaration_specifier
    : storage_class_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | type_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | type_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | function_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

storage_class_specifier
    : TYPEDEF { $$ = ast_place(ast_named(AST_SPECIFIER, "typedef"), @$.first_line, @$.first_column); }
    | EXTERN { $$ = ast_place(ast_named(AST_SPECIFIER, "extern"), @$.first_line, @$.first_column); }
    | STATIC { $$ = ast_place(ast_named(AST_SPECIFIER, "static"), @$.first_line, @$.first_column); }
    | AUTO { $$ = ast_place(ast_named(AST_SPECIFIER, "auto"), @$.first_line, @$.first_column); }
    | REGISTER { $$ = ast_place(ast_named(AST_SPECIFIER, "register"), @$.first_line, @$.first_column); }
    ;

type_specifier
    : VOID { $$ = ast_place(ast_named(AST_SPECIFIER, "void"), @$.first_line, @$.first_column); }
    | CHAR { $$ = ast_place(ast_named(AST_SPECIFIER, "char"), @$.first_line, @$.first_column); }
    | SHORT { $$ = ast_place(ast_named(AST_SPECIFIER, "short"), @$.first_line, @$.first_column); }
    | INT { $$ = ast_place(ast_named(AST_SPECIFIER, "int"), @$.first_line, @$.first_column); }
    | LONG { $$ = ast_place(ast_named(AST_SPECIFIER, "long"), @$.first_line, @$.first_column); }
    | FLOAT { $$ = ast_place(ast_named(AST_SPECIFIER, "float"), @$.first_line, @$.first_column); }
    | DOUBLE { $$ = ast_place(ast_named(AST_SPECIFIER, "double"), @$.first_line, @$.first_column); }
    | SIGNED { $$ = ast_place(ast_named(AST_SPECIFIER, "signed"), @$.first_line, @$.first_column); }
    | UNSIGNED { $$ = ast_place(ast_named(AST_SPECIFIER, "unsigned"), @$.first_line, @$.first_column); }
    | BOOL { $$ = ast_place(ast_named(AST_SPECIFIER, "bool"), @$.first_line, @$.first_column); }
    | struct_or_union_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | enum_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | class_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | TYPEDEF_NAME { $$ = ast_place(ast_text(AST_TYPE_NAME, $1), @$.first_line, @$.first_column); }
    ;

struct_or_union_specifier
    : struct_or_union IDENTIFIER '{' struct_declaration_list '}' {
        $$ = ast_place(ast_make((AstKind) $1, $2, $4, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | struct_or_union '{' struct_declaration_list '}' {
        $$ = ast_place(ast_make((AstKind) $1, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | struct_or_union IDENTIFIER {
        $$ = ast_place(ast_make((AstKind) $1, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

struct_or_union
    : STRUCT { $$ = AST_STRUCT; }
    | UNION { $$ = AST_UNION; }
    ;

struct_declaration_list
    : struct_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | struct_declaration_list struct_declaration { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

/* Every member has a declarator, so the struct is non-empty and C99. */
struct_declaration
    : specifier_qualifier_list struct_declarator_list ';' {
        $$ = ast_place(ast_make(AST_DECLARATION, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    | error ';' { yyerrok; $$ = NULL; }
    ;

specifier_qualifier_list
    : specifier_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | specifier_qualifier_list specifier_qualifier { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

specifier_qualifier
    : type_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | type_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

struct_declarator_list
    : struct_declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | struct_declarator_list ',' struct_declarator { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

/* A bit-field is a struct or union member. A ':' after a class name is
 * still the base-clause, and a ':' after a constructor is its initializer. */
struct_declarator
    : declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | declarator ':' constant_expression {
        $$ = ast_place(ast_make(AST_BITFIELD, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

enum_specifier
    : ENUM '{' enumerator_list '}' { $$ = ast_place(ast_make(AST_ENUM, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | ENUM '{' enumerator_list ',' '}' { $$ = ast_place(ast_make(AST_ENUM, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | ENUM IDENTIFIER '{' enumerator_list '}' {
        $$ = ast_place(ast_make(AST_ENUM, $2, $4, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | ENUM IDENTIFIER '{' enumerator_list ',' '}' {
        $$ = ast_place(ast_make(AST_ENUM, $2, $4, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | ENUM IDENTIFIER { $$ = ast_place(ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

enumerator_list
    : enumerator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | enumerator_list ',' enumerator { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

enumerator
    : IDENTIFIER { $$ = ast_place(ast_text(AST_ENUMERATOR, $1), @$.first_line, @$.first_column); }
    | IDENTIFIER '=' constant_expression {
        $$ = ast_place(ast_make(AST_ENUMERATOR, $1, $3, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

/*
 * class Name : access base, access base { members }
 * More than one base is allowed. public:, private:, and protected: are
 * the C++ access specifiers. The scanner records the class name so it
 * can later be a type name.
 *
 * A constructor is Name(parameters), with no return type, optionally
 * followed by : member(expr), base(expr) and a body. A destructor is
 * ~Name(parameters). Out of line they are Name::Name and Name::~Name.
 * Inside a class, the scanner returns the class name as CTOR_NAME when
 * it starts a member and the next token is '('. That keeps the
 * constructor distinct from a declaration that begins with a type name.
 */
class_specifier
    : CLASS name { smallcpp_push_class($2->text); } base_clause '{' class_member_list_opt '}' {
        smallcpp_pop_class();
        $$ = ast_place(ast_make(AST_CLASS, NULL, $2, $4, $6, NULL), @$.first_line, @$.first_column);
      }
    | CLASS name { smallcpp_push_class($2->text); } '{' class_member_list_opt '}' {
        smallcpp_pop_class();
        $$ = ast_place(ast_make(AST_CLASS, NULL, $2, NULL, $5, NULL), @$.first_line, @$.first_column);
      }
    | CLASS name { $$ = ast_place(ast_make(AST_CLASS, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | CLASS '{' class_member_list_opt '}' {
        $$ = ast_place(ast_make(AST_CLASS, NULL, NULL, NULL, $3, NULL), @$.first_line, @$.first_column);
      }
    ;

name
    : IDENTIFIER { $$ = ast_place(ast_text(AST_IDENT, $1), @$.first_line, @$.first_column); }
    | TYPEDEF_NAME { $$ = ast_place(ast_text(AST_TYPE_NAME, $1), @$.first_line, @$.first_column); }
    ;

base_clause
    : ':' base_specifier_list { $$ = ast_place($2, @$.first_line, @$.first_column); }
    ;

base_specifier_list
    : base_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | base_specifier_list ',' base_specifier { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

base_specifier
    : access_keyword name { $$ = ast_place(ast_make(AST_BASE, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column); }
    | name { $$ = ast_place(ast_make(AST_BASE, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | VIRTUAL access_keyword name { $$ = ast_place(ast_make(AST_BASE, strdup("virtual"), $2, $3, NULL, NULL), @$.first_line, @$.first_column); }
    | access_keyword VIRTUAL name { $$ = ast_place(ast_make(AST_BASE, strdup("virtual"), $1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    | VIRTUAL name { $$ = ast_place(ast_make(AST_BASE, strdup("virtual"), $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

access_keyword
    : PUBLIC { $$ = ast_place(ast_named(AST_ACCESS, "public"), @$.first_line, @$.first_column); }
    | PRIVATE { $$ = ast_place(ast_named(AST_ACCESS, "private"), @$.first_line, @$.first_column); }
    | PROTECTED { $$ = ast_place(ast_named(AST_ACCESS, "protected"), @$.first_line, @$.first_column); }
    ;

class_member_list_opt
    : %empty { $$ = NULL; }
    | class_member_list { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

class_member_list
    : class_member { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | class_member_list class_member { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

class_member
    : access_keyword ':' { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | constructor_member { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | destructor_member { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | function_definition { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | using_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | error ';' { yyerrok; $$ = NULL; }
    ;

constructor_member
    : CTOR_NAME '(' ')' ctor_initializer_opt compound_statement {
        $$ = ast_place(ctor_spelling($1, NULL, $4, $5), @$.first_line, @$.first_column);
      }
    | CTOR_NAME '(' ')' ';' { $$ = ast_place(ctor_spelling($1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | CTOR_NAME '(' parameter_type_list ')' ctor_initializer_opt compound_statement {
        $$ = ast_place(ctor_spelling($1, $3, $5, $6), @$.first_line, @$.first_column);
      }
    | CTOR_NAME '(' parameter_type_list ')' ';' { $$ = ast_place(ctor_spelling($1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    ;

destructor_params
    : '(' ')' { $$ = NULL; }
    | '(' parameter_type_list ')' { $$ = ast_place($2, @$.first_line, @$.first_column); }
    ;

destructor_member
    : '~' name destructor_params compound_statement {
        if (!text_is($2, smallcpp_current_class()))
            yylloc = @$, yyerror("destructor name does not match the class");
        $$ = ast_place(make_dtor($2, $3, $4, 0), @$.first_line, @$.first_column);
      }
    | '~' name destructor_params ';' {
        if (!text_is($2, smallcpp_current_class()))
            yylloc = @$, yyerror("destructor name does not match the class");
        $$ = ast_place(make_dtor($2, $3, NULL, 0), @$.first_line, @$.first_column);
      }
    | VIRTUAL '~' name destructor_params compound_statement {
        if (!text_is($3, smallcpp_current_class()))
            yylloc = @$, yyerror("destructor name does not match the class");
        $$ = ast_place(make_dtor($3, $4, $5, 1), @$.first_line, @$.first_column);
      }
    | VIRTUAL '~' name destructor_params ';' {
        if (!text_is($3, smallcpp_current_class()))
            yylloc = @$, yyerror("destructor name does not match the class");
        $$ = ast_place(make_dtor($3, $4, NULL, 1), @$.first_line, @$.first_column);
      }
    ;

constructor_definition
    : name SCOPE name '(' ')' ctor_initializer_opt compound_statement {
        if (!names_equal($1, $3))
            yylloc = @$, yyerror("constructor name does not match the class");
        ast_free($1);
        $$ = ast_place(make_ctor($3, NULL, $6, $7), @$.first_line, @$.first_column);
      }
    | name SCOPE name '(' parameter_type_list ')' ctor_initializer_opt compound_statement {
        if (!names_equal($1, $3))
            yylloc = @$, yyerror("constructor name does not match the class");
        ast_free($1);
        $$ = ast_place(make_ctor($3, $5, $7, $8), @$.first_line, @$.first_column);
      }
    ;

destructor_definition
    : name SCOPE '~' name destructor_params compound_statement {
        if (!names_equal($1, $4))
            yylloc = @$, yyerror("destructor name does not match the class");
        ast_free($1);
        $$ = ast_place(make_dtor($4, $5, $6, 0), @$.first_line, @$.first_column);
      }
    | name SCOPE '~' name destructor_params ';' {
        if (!names_equal($1, $4))
            yylloc = @$, yyerror("destructor name does not match the class");
        ast_free($1);
        $$ = ast_place(make_dtor($4, $5, NULL, 0), @$.first_line, @$.first_column);
      }
    ;

ctor_initializer_opt
    : %empty { $$ = NULL; }
    | ':' mem_initializer_list { $$ = ast_place($2, @$.first_line, @$.first_column); }
    ;

mem_initializer_list
    : mem_initializer { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | mem_initializer_list ',' mem_initializer { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

mem_initializer
    : name '(' ')' { $$ = ast_place(make_mem_init($1, NULL), @$.first_line, @$.first_column); }
    | name '(' argument_expression_list ')' { $$ = ast_place(make_mem_init($1, $3), @$.first_line, @$.first_column); }
    ;

using_declaration
    : USING NAMESPACE name ';' {
        $$ = ast_place(ast_make(AST_USING, strdup("namespace"), $3, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | USING using_name ';' { $$ = ast_place(ast_make(AST_USING, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

using_name
    : name { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | nested_name { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

type_qualifier
    : CONST { $$ = ast_place(ast_named(AST_SPECIFIER, "const"), @$.first_line, @$.first_column); }
    | VOLATILE { $$ = ast_place(ast_named(AST_SPECIFIER, "volatile"), @$.first_line, @$.first_column); }
    ;

function_specifier
    : INLINE { $$ = ast_place(ast_named(AST_SPECIFIER, "inline"), @$.first_line, @$.first_column); }
    | VIRTUAL { $$ = ast_place(ast_named(AST_SPECIFIER, "virtual"), @$.first_line, @$.first_column); }
    ;

init_declarator_list
    : init_declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | init_declarator_list ',' init_declarator { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

init_declarator
    : declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | declarator '=' initializer { $$ = ast_place(ast_make(AST_INIT, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    ;

/* Brace initialization is written after '='. T x{...} is not supported.
 * .member = value and [index] = value are designated initializers. */
initializer
    : assignment_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | brace_initializer { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

brace_initializer
    : '{' '}' { $$ = ast_place(ast_make(AST_INIT, NULL, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '{' initializer_list '}' { $$ = ast_place(ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '{' initializer_list ',' '}' { $$ = ast_place(ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

initializer_list
    : initializer { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | designator_list '=' initializer { $$ = ast_place(designate($1, $3), @$.first_line, @$.first_column); }
    | initializer_list ',' initializer { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    | initializer_list ',' designator_list '=' initializer {
        ast_append($1, designate($3, $5));
        $$ = ast_place($1, @$.first_line, @$.first_column);
      }
    ;

designator_list
    : designator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | designator_list designator { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

designator
    : '.' IDENTIFIER { $$ = ast_place(ast_make(AST_DESIGNATED, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '[' constant_expression ']' { $$ = ast_place(ast_make(AST_DESIGNATED, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

/* Object and function declarators. No empty brackets, so no flexible array. */
declarator
    : pointer direct_declarator { $$ = ast_place(ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column); }
    | direct_declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

direct_declarator
    : declarator_id { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | '(' declarator ')' { $$ = ast_place($2, @$.first_line, @$.first_column); }
    | direct_declarator '[' constant_expression ']' {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_declarator '(' ')' cv_qualifier_opt pure_specifier_opt {
        $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $1, NULL, $4, $5), @$.first_line, @$.first_column);
      }
    | direct_declarator '(' parameter_type_list ')' cv_qualifier_opt pure_specifier_opt {
        $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $1, $3, $5, $6), @$.first_line, @$.first_column);
      }
    ;

/* A pure virtual method is = 0. Any other constant is rejected. */
pure_specifier_opt
    : %empty %prec LOWER_THAN_PURE { $$ = NULL; }
    | '=' CONSTANT {
        int pure = $2 != NULL && strcmp($2, "0") == 0;
        if (!pure)
            yylloc = @$, yyerror("pure specifier is = 0");
        free($2);
        $$ = ast_place(pure ? ast_named(AST_SPECIFIER, "pure") : NULL, @$.first_line, @$.first_column);
      }
    ;

/* const and volatile after a parameter list. No ref-qualifiers. */
cv_qualifier_opt
    : %empty { $$ = NULL; }
    | cv_qualifier_seq { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

cv_qualifier_seq
    : type_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | cv_qualifier_seq type_qualifier { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

/* Point::move is qualified. The last component is an identifier, not a type. */
declarator_id
    : IDENTIFIER { $$ = ast_place(ast_text(AST_IDENT, $1), @$.first_line, @$.first_column); }
    | name SCOPE declarator_id { $$ = ast_place(ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    ;

pointer
    : '*' { $$ = ast_place(ast_named(AST_POINTER, "*"), @$.first_line, @$.first_column); }
    | '*' type_qualifier_list { $$ = ast_place(ast_make(AST_POINTER, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '*' pointer { $$ = ast_place(ast_make(AST_POINTER, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '*' type_qualifier_list pointer { $$ = ast_place(ast_make(AST_POINTER, NULL, $2, $3, NULL, NULL), @$.first_line, @$.first_column); }
    ;

type_qualifier_list
    : type_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | type_qualifier_list type_qualifier { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

parameter_type_list
    : parameter_list { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | parameter_list ',' ELLIPSIS {
        ast_append($1, ast_named(AST_ELLIPSIS, "..."));
        $$ = ast_place($1, @$.first_line, @$.first_column);
      }
    ;

parameter_list
    : parameter_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | parameter_list ',' parameter_declaration { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

parameter_declaration
    : parameter_specifiers { $$ = ast_place(ast_make(AST_PARAMETER, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | parameter_specifiers parameter_declarator {
        note_declarator($2);
        $$ = ast_place(ast_make(AST_PARAMETER, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    | parameter_specifiers parameter_declarator '=' assignment_expression {
        note_declarator($2);
        $$ = ast_place(ast_make(AST_PARAMETER, NULL, $1, $2, $4, NULL), @$.first_line, @$.first_column);
      }
    ;

/* register is the only storage class both languages allow on a parameter. */
parameter_specifiers
    : parameter_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | parameter_specifiers parameter_specifier { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

parameter_specifier
    : type_specifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | type_qualifier { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | REGISTER { $$ = ast_place(ast_named(AST_SPECIFIER, "register"), @$.first_line, @$.first_column); }
    ;

/* Named and abstract parameter declarators. Empty [] adjusts to a pointer. */
parameter_declarator
    : pointer direct_parameter_declarator {
        $$ = ast_place(ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_parameter_declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | pointer { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

direct_parameter_declarator
    : IDENTIFIER { $$ = ast_place(ast_text(AST_IDENT, $1), @$.first_line, @$.first_column); }
    | '(' parameter_declarator ')' { $$ = ast_place($2, @$.first_line, @$.first_column); }
    | '[' ']' { $$ = ast_place(ast_named(AST_ARRAY, "[]"), @$.first_line, @$.first_column); }
    | '[' constant_expression ']' { $$ = ast_place(ast_make(AST_ARRAY, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '(' ')' { $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '(' parameter_type_list ')' { $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | direct_parameter_declarator '[' ']' {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_parameter_declarator '[' constant_expression ']' {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_parameter_declarator '(' parameter_type_list ')' {
        $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

type_name
    : specifier_qualifier_list { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | specifier_qualifier_list abstract_declarator {
        $$ = ast_place(ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

abstract_declarator
    : pointer { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | direct_abstract_declarator { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | pointer direct_abstract_declarator {
        $$ = ast_place(ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

direct_abstract_declarator
    : '(' abstract_declarator ')' { $$ = ast_place($2, @$.first_line, @$.first_column); }
    | '[' ']' { $$ = ast_place(ast_named(AST_ARRAY, "[]"), @$.first_line, @$.first_column); }
    | '[' constant_expression ']' { $$ = ast_place(ast_make(AST_ARRAY, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '(' ')' { $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '(' parameter_type_list ')' { $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | direct_abstract_declarator '[' ']' {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_abstract_declarator '[' constant_expression ']' {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    | direct_abstract_declarator '(' parameter_type_list ')' {
        $$ = ast_place(ast_make(AST_FUNCTION_DECL, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

statement
    : labeled_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | compound_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | expression_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | selection_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | iteration_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | jump_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | try_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | throw_statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | error ';' { yyerrok; $$ = NULL; }
    ;

labeled_statement
    : IDENTIFIER ':' statement { $$ = ast_place(ast_make(AST_LABEL, $1, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | CASE constant_expression ':' statement {
        $$ = ast_place(ast_make(AST_CASE, NULL, $2, $4, NULL, NULL), @$.first_line, @$.first_column);
      }
    | DEFAULT ':' statement { $$ = ast_place(ast_make(AST_DEFAULT, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

compound_statement
    : '{' '}' { $$ = ast_place(ast_make(AST_COMPOUND, NULL, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | '{' block_item_list '}' { $$ = ast_place(ast_make(AST_COMPOUND, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

block_item_list
    : block_item { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | block_item_list block_item { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

block_item
    : declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | statement { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | using_declaration { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

expression_statement
    : ';' { $$ = ast_place(ast_named(AST_EXPR_STMT, ";"), @$.first_line, @$.first_column); }
    | expression ';' { $$ = ast_place(ast_make(AST_EXPR_STMT, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE {
        $$ = ast_place(ast_make(AST_IF, NULL, $3, $5, NULL, NULL), @$.first_line, @$.first_column);
      }
    | IF '(' expression ')' statement ELSE statement {
        $$ = ast_place(ast_make(AST_IF, NULL, $3, $5, $7, NULL), @$.first_line, @$.first_column);
      }
    | SWITCH '(' expression ')' statement {
        $$ = ast_place(ast_make(AST_SWITCH, NULL, $3, $5, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

iteration_statement
    : WHILE '(' expression ')' statement {
        $$ = ast_place(ast_make(AST_WHILE, NULL, $3, $5, NULL, NULL), @$.first_line, @$.first_column);
      }
    | DO statement WHILE '(' expression ')' ';' {
        $$ = ast_place(ast_make(AST_DO, NULL, $2, $5, NULL, NULL), @$.first_line, @$.first_column);
      }
    | FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement {
        $$ = ast_place(ast_make(AST_FOR, NULL, $3, $5, $7, $9), @$.first_line, @$.first_column);
      }
    | FOR '(' declaration expression_opt ';' expression_opt ')' statement {
        $$ = ast_place(ast_make(AST_FOR, NULL, $3, $4, $6, $8), @$.first_line, @$.first_column);
      }
    ;

jump_statement
    : GOTO IDENTIFIER ';' { $$ = ast_place(ast_make(AST_GOTO, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | CONTINUE ';' { $$ = ast_place(ast_named(AST_CONTINUE, "continue"), @$.first_line, @$.first_column); }
    | BREAK ';' { $$ = ast_place(ast_named(AST_BREAK, "break"), @$.first_line, @$.first_column); }
    | RETURN ';' { $$ = ast_place(ast_named(AST_RETURN, "return"), @$.first_line, @$.first_column); }
    | RETURN expression ';' { $$ = ast_place(ast_make(AST_RETURN, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

/*
 * try compound-statement catch-clause+
 * A catch names one parameter, or it is catch (...), which catches every
 * type. The ... is not a parameter and is not the variadic-parameter marker.
 * throw expression; is the C++ throw statement.
 */
try_statement
    : TRY compound_statement catch_clause_list {
        $$ = ast_place(ast_make(AST_TRY, NULL, $2, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

catch_clause_list
    : catch_clause { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | catch_clause_list catch_clause { ast_append($1, $2); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

catch_clause
    : CATCH '(' parameter_declaration ')' compound_statement {
        $$ = ast_place(ast_make(AST_CATCH, NULL, $3, $5, NULL, NULL), @$.first_line, @$.first_column);
      }
    | CATCH '(' ELLIPSIS ')' compound_statement { $$ = ast_place(ast_catch_all($5), @$.first_line, @$.first_column); }
    ;

throw_statement
    : THROW assignment_expression ';' { $$ = ast_place(ast_make(AST_THROW, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | THROW ';' { $$ = ast_place(ast_named(AST_THROW, "throw"), @$.first_line, @$.first_column); }
    ;

primary_expression
    : IDENTIFIER { $$ = ast_place(ast_text(AST_IDENT, $1), @$.first_line, @$.first_column); }
    | nested_name { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | THIS { $$ = ast_place(ast_named(AST_THIS, "this"), @$.first_line, @$.first_column); }
    | TRUE { $$ = ast_place(ast_named(AST_LITERAL, "true"), @$.first_line, @$.first_column); }
    | FALSE { $$ = ast_place(ast_named(AST_LITERAL, "false"), @$.first_line, @$.first_column); }
    | NULLPTR { $$ = ast_place(ast_null("nullptr"), @$.first_line, @$.first_column); }
    | NULL_KEYWORD { $$ = ast_place(ast_null("NULL"), @$.first_line, @$.first_column); }
    | CONSTANT { $$ = ast_place(ast_text(AST_LITERAL, $1), @$.first_line, @$.first_column); }
    | STRING_LITERAL { $$ = ast_place(ast_text(AST_LITERAL, $1), @$.first_line, @$.first_column); }
    | '(' expression ')' { $$ = ast_place($2, @$.first_line, @$.first_column); }
    | format_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

/*
 * print and format are native. The first operand is a string literal.
 * Replacement fields use C++23 syntax: {}, {0}, {:.2f}, and the other
 * standard format specs. The arguments follow the string.
 */
format_expression
    : PRINT '(' STRING_LITERAL ')' { yylloc = @$; $$ = ast_place(native_format(AST_PRINT, $3, NULL), @$.first_line, @$.first_column); }
    | PRINT '(' STRING_LITERAL ',' argument_expression_list ')' {
        yylloc = @$; $$ = ast_place(native_format(AST_PRINT, $3, $5), @$.first_line, @$.first_column);
      }
    | FORMAT '(' STRING_LITERAL ')' { yylloc = @$; $$ = ast_place(native_format(AST_FORMAT, $3, NULL), @$.first_line, @$.first_column); }
    | FORMAT '(' STRING_LITERAL ',' argument_expression_list ')' {
        yylloc = @$; $$ = ast_place(native_format(AST_FORMAT, $3, $5), @$.first_line, @$.first_column);
      }
    ;

/* A nested name has at least one '::', so a bare type name is not an expression. */
nested_name
    : name SCOPE name { $$ = ast_place(ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    | nested_name SCOPE name { $$ = ast_place(ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column); }
    ;

postfix_expression
    : primary_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | postfix_expression '[' expression ']' {
        $$ = ast_place(ast_make(AST_INDEX, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    | postfix_expression '(' ')' { $$ = ast_place(ast_make(AST_CALL, NULL, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | postfix_expression '(' argument_expression_list ')' {
        $$ = ast_place(ast_make(AST_CALL, NULL, $1, $3, NULL, NULL), @$.first_line, @$.first_column);
      }
    | postfix_expression '.' IDENTIFIER { $$ = ast_place(ast_make(AST_MEMBER, $3, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | postfix_expression '.' '~' name '(' ')' { $$ = ast_place(dtor_call($1, $4, NULL, 0), @$.first_line, @$.first_column); }
    | postfix_expression '.' '~' name '(' argument_expression_list ')' {
        $$ = ast_place(dtor_call($1, $4, $6, 0), @$.first_line, @$.first_column);
      }
    | postfix_expression ARROW IDENTIFIER { $$ = ast_place(ast_make(AST_ARROW, $3, $1, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | postfix_expression ARROW '~' name '(' ')' { $$ = ast_place(dtor_call($1, $4, NULL, 1), @$.first_line, @$.first_column); }
    | postfix_expression ARROW '~' name '(' argument_expression_list ')' {
        $$ = ast_place(dtor_call($1, $4, $6, 1), @$.first_line, @$.first_column);
      }
    | postfix_expression INC { $$ = ast_place(ast_unary("post++", $1), @$.first_line, @$.first_column); }
    | postfix_expression DEC { $$ = ast_place(ast_unary("post--", $1), @$.first_line, @$.first_column); }
    ;

argument_expression_list
    : assignment_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | argument_expression_list ',' assignment_expression { ast_append($1, $3); $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

unary_expression
    : postfix_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | INC unary_expression { $$ = ast_place(ast_unary("++", $2), @$.first_line, @$.first_column); }
    | DEC unary_expression { $$ = ast_place(ast_unary("--", $2), @$.first_line, @$.first_column); }
    | unary_operator cast_expression { $$ = ast_place(ast_unary($1, $2), @$.first_line, @$.first_column); }
    | SIZEOF unary_expression { $$ = ast_place(ast_make(AST_SIZEOF, NULL, $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | SIZEOF '(' type_name ')' { $$ = ast_place(ast_make(AST_SIZEOF, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | TYPEID '(' expression ')' { $$ = ast_place(ast_make(AST_TYPEID, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | TYPEID '(' type_name ')' { $$ = ast_place(ast_make(AST_TYPEID, NULL, $3, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | NEW new_type %prec NEW_BARE { $$ = ast_place(make_new($2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | NEW new_type new_pointer { $$ = ast_place(make_new($2, $3, NULL, NULL), @$.first_line, @$.first_column); }
    | NEW new_type new_arrays { $$ = ast_place(make_new($2, NULL, $3, NULL), @$.first_line, @$.first_column); }
    | NEW new_type new_pointer new_arrays { $$ = ast_place(make_new($2, $3, $4, NULL), @$.first_line, @$.first_column); }
    | NEW new_type '(' ')' { $$ = ast_place(make_new($2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | NEW new_type '(' argument_expression_list ')' { $$ = ast_place(make_new($2, NULL, NULL, $4), @$.first_line, @$.first_column); }
    | NEW new_type new_pointer '(' ')' { $$ = ast_place(make_new($2, $3, NULL, NULL), @$.first_line, @$.first_column); }
    | NEW new_type new_pointer '(' argument_expression_list ')' {
        $$ = ast_place(make_new($2, $3, NULL, $5), @$.first_line, @$.first_column);
      }
    | DELETE cast_expression {
        if (!operand_is_pointer($2))
            yylloc = @$, yyerror("delete operand is not a pointer");
        $$ = ast_place(ast_delete_noop($2), @$.first_line, @$.first_column);
      }
    ;

/*
 * new takes a class or typedef name, or one type specifier, then an
 * optional pointer declarator and an optional array bound. '*' and '['
 * belong to the new-expression. (new int) * p is multiplication.
 */
new_type
    : name { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | VOID { $$ = ast_place(ast_named(AST_SPECIFIER, "void"), @$.first_line, @$.first_column); }
    | CHAR { $$ = ast_place(ast_named(AST_SPECIFIER, "char"), @$.first_line, @$.first_column); }
    | SHORT { $$ = ast_place(ast_named(AST_SPECIFIER, "short"), @$.first_line, @$.first_column); }
    | INT { $$ = ast_place(ast_named(AST_SPECIFIER, "int"), @$.first_line, @$.first_column); }
    | LONG { $$ = ast_place(ast_named(AST_SPECIFIER, "long"), @$.first_line, @$.first_column); }
    | FLOAT { $$ = ast_place(ast_named(AST_SPECIFIER, "float"), @$.first_line, @$.first_column); }
    | DOUBLE { $$ = ast_place(ast_named(AST_SPECIFIER, "double"), @$.first_line, @$.first_column); }
    | SIGNED { $$ = ast_place(ast_named(AST_SPECIFIER, "signed"), @$.first_line, @$.first_column); }
    | UNSIGNED { $$ = ast_place(ast_named(AST_SPECIFIER, "unsigned"), @$.first_line, @$.first_column); }
    | BOOL { $$ = ast_place(ast_named(AST_SPECIFIER, "bool"), @$.first_line, @$.first_column); }
    | STRUCT IDENTIFIER { $$ = ast_place(ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | STRUCT TYPEDEF_NAME { $$ = ast_place(ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | ENUM IDENTIFIER { $$ = ast_place(ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    | ENUM TYPEDEF_NAME { $$ = ast_place(ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

new_pointer
    : '*' { $$ = ast_place(ast_named(AST_POINTER, "*"), @$.first_line, @$.first_column); }
    | '*' new_pointer { $$ = ast_place(ast_make(AST_POINTER, strdup("*"), $2, NULL, NULL, NULL), @$.first_line, @$.first_column); }
    ;

new_arrays
    : '[' expression ']' { $$ = ast_place(ast_make(AST_ARRAY, NULL, NULL, $2, NULL, NULL), @$.first_line, @$.first_column); }
    | '[' expression ']' new_arrays {
        $$ = ast_place(ast_make(AST_ARRAY, NULL, $4, $2, NULL, NULL), @$.first_line, @$.first_column);
      }
    ;

unary_operator
    : '&' { $$ = "&"; }
    | '*' { $$ = "*"; }
    | '+' { $$ = "+"; }
    | '-' { $$ = "-"; }
    | '~' { $$ = "~"; }
    | '!' { $$ = "!"; }
    ;

cast_expression
    : unary_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | '(' type_name ')' cast_expression { $$ = ast_place(ast_make(AST_CAST, NULL, $2, $4, NULL, NULL), @$.first_line, @$.first_column); }
    ;

multiplicative_expression
    : cast_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | multiplicative_expression '*' cast_expression { $$ = ast_place(ast_binary($1, "*", $3), @$.first_line, @$.first_column); }
    | multiplicative_expression '/' cast_expression { $$ = ast_place(ast_binary($1, "/", $3), @$.first_line, @$.first_column); }
    | multiplicative_expression '%' cast_expression { $$ = ast_place(ast_binary($1, "%", $3), @$.first_line, @$.first_column); }
    ;

additive_expression
    : multiplicative_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | additive_expression '+' multiplicative_expression { $$ = ast_place(ast_binary($1, "+", $3), @$.first_line, @$.first_column); }
    | additive_expression '-' multiplicative_expression { $$ = ast_place(ast_binary($1, "-", $3), @$.first_line, @$.first_column); }
    ;

shift_expression
    : additive_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | shift_expression SHL additive_expression { $$ = ast_place(ast_binary($1, "<<", $3), @$.first_line, @$.first_column); }
    | shift_expression SHR additive_expression { $$ = ast_place(ast_binary($1, ">>", $3), @$.first_line, @$.first_column); }
    ;

relational_expression
    : shift_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | relational_expression '<' shift_expression { $$ = ast_place(ast_binary($1, "<", $3), @$.first_line, @$.first_column); }
    | relational_expression '>' shift_expression { $$ = ast_place(ast_binary($1, ">", $3), @$.first_line, @$.first_column); }
    | relational_expression LE shift_expression { $$ = ast_place(ast_binary($1, "<=", $3), @$.first_line, @$.first_column); }
    | relational_expression GE shift_expression { $$ = ast_place(ast_binary($1, ">=", $3), @$.first_line, @$.first_column); }
    ;

equality_expression
    : relational_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | equality_expression EQEQ relational_expression { $$ = ast_place(ast_binary($1, "==", $3), @$.first_line, @$.first_column); }
    | equality_expression NOTEQ relational_expression { $$ = ast_place(ast_binary($1, "!=", $3), @$.first_line, @$.first_column); }
    ;

and_expression
    : equality_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | and_expression '&' equality_expression { $$ = ast_place(ast_binary($1, "&", $3), @$.first_line, @$.first_column); }
    ;

exclusive_or_expression
    : and_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | exclusive_or_expression '^' and_expression { $$ = ast_place(ast_binary($1, "^", $3), @$.first_line, @$.first_column); }
    ;

inclusive_or_expression
    : exclusive_or_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | inclusive_or_expression '|' exclusive_or_expression { $$ = ast_place(ast_binary($1, "|", $3), @$.first_line, @$.first_column); }
    ;

logical_and_expression
    : inclusive_or_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | logical_and_expression ANDAND inclusive_or_expression { $$ = ast_place(ast_binary($1, "&&", $3), @$.first_line, @$.first_column); }
    ;

logical_or_expression
    : logical_and_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | logical_or_expression OROR logical_and_expression { $$ = ast_place(ast_binary($1, "||", $3), @$.first_line, @$.first_column); }
    ;

conditional_expression
    : logical_or_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | logical_or_expression '?' expression ':' conditional_expression {
        $$ = ast_place(ast_make(AST_CONDITIONAL, NULL, $1, $3, $5, NULL), @$.first_line, @$.first_column);
      }
    ;

assignment_expression
    : conditional_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | unary_expression assignment_operator assignment_expression {
        $$ = ast_place(ast_assign($1, $2, $3), @$.first_line, @$.first_column);
      }
    ;

assignment_operator
    : '=' { $$ = "="; }
    | ADD_ASSIGN { $$ = "+="; }
    | SUB_ASSIGN { $$ = "-="; }
    | MUL_ASSIGN { $$ = "*="; }
    | DIV_ASSIGN { $$ = "/="; }
    | MOD_ASSIGN { $$ = "%="; }
    | SHL_ASSIGN { $$ = "<<="; }
    | SHR_ASSIGN { $$ = ">>="; }
    | AND_ASSIGN { $$ = "&="; }
    | XOR_ASSIGN { $$ = "^="; }
    | OR_ASSIGN { $$ = "|="; }
    ;

expression
    : assignment_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    | expression ',' assignment_expression { $$ = ast_place(ast_binary($1, ",", $3), @$.first_line, @$.first_column); }
    ;

expression_opt
    : %empty { $$ = NULL; }
    | expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

constant_expression
    : conditional_expression { $$ = ast_place($1, @$.first_line, @$.first_column); }
    ;

%%

static int real_token(const char *name)
{
    if (name == NULL || name[0] == '\0')
        return 0;
    if (strcmp(name, "LOWER_THAN_ELSE") == 0 || strcmp(name, "LOWER_THAN_PURE") == 0
        || strcmp(name, "NEW_BARE") == 0)
        return 0;
    return 1;
}

static void append_text(char **buf, size_t *length, size_t *cap, const char *text)
{
    size_t add;
    char *grown;

    if (text == NULL)
        return;
    add = strlen(text);
    if (*length + add + 1 > *cap) {
        size_t next = *cap == 0 ? 64 : *cap;
        while (*length + add + 1 > next)
            next *= 2;
        grown = realloc(*buf, next);
        if (grown == NULL)
            return;
        *buf = grown;
        *cap = next;
    }
    memcpy(*buf + *length, text, add);
    *length += add;
    (*buf)[*length] = '\0';
}

static int
yyreport_syntax_error(const yypcontext_t *ctx)
{
    YYLTYPE *loc = yypcontext_location(ctx);
    yysymbol_kind_t token = yypcontext_token(ctx);
    int count = yypcontext_expected_tokens(ctx, NULL, 0);
    yysymbol_kind_t *expected = NULL;
    int named = 0;
    char *message = NULL;
    size_t length = 0;
    size_t cap = 0;
    int i;

    if (loc != NULL)
        yylloc = *loc;
    if (token == YYSYMBOL_YYEMPTY)
        append_text(&message, &length, &cap, "syntax error");
    else {
        append_text(&message, &length, &cap, "syntax error, unexpected ");
        append_text(&message, &length, &cap, yysymbol_name(token));
    }
    if (count > 0) {
        expected = calloc((size_t) count, sizeof *expected);
        if (expected != NULL)
            count = yypcontext_expected_tokens(ctx, expected, count);
        else
            count = 0;
    } else {
        count = 0;
    }
    for (i = 0; i < count; i++) {
        const char *name = yysymbol_name(expected[i]);
        if (!real_token(name))
            continue;
        if (named == 0)
            append_text(&message, &length, &cap, ", expected ");
        else
            append_text(&message, &length, &cap, ", ");
        append_text(&message, &length, &cap, name);
        named++;
    }
    yyerror(message != NULL ? message : "syntax error");
    free(message);
    free(expected);
    return 0;
}
