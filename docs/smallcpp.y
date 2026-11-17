/*
 * smallcpp grammar. Start symbol: translation_unit.
 *
 * The base language is the syntax shared by ISO C99 and ISO C++11, so a
 * program that stays in that subset is acceptable to a modern C compiler
 * and a modern C++ compiler. smallcpp adds classes, inheritance,
 * exceptions, namespaces, and precise garbage collection. A class may
 * list more than one base. catch (...) catches every exception type.
 * That ... is not a parameter. A constructor is a method whose name is
 * the class name and that has no return type. A destructor is
 * ~ClassName(), also with no return type. A constructor may have an
 * initializer list: ClassName(args) : member(expr), base(expr) { ... }.
 * A parameter may have a default, written name = expression. using name;
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
 * true, and false are the plain spellings, not macros. namespace uses
 * C++ syntax:
 * namespace name { ... }.
 * A qualified name uses '::'. A new-expression creates a pointer. delete
 * type-checks its pointer and calls the destructor. It does not deallocate.
 * A direct call is object.~Class() or pointer->~Class(). nullptr and NULL are keywords for
 * the same null pointer constant.
 * Source files use the .scpp extension. Ordinary preprocessing is already
 * done. Semantic actions build a syntax tree. They do not generate code.
 *
 * A syntax error is reported with its line and column, then the error
 * token resynchronizes at the next semicolon so later errors are still
 * reported. That recovery is not part of the language.
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

static struct AstNode *make_dtor(struct AstNode *name, struct AstNode *body)
{
    return ast_make(AST_DESTRUCTOR, steal_text(name), body, NULL, NULL, NULL);
}

static struct AstNode *dtor_call(struct AstNode *object, struct AstNode *name, int arrow)
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
                    NULL, NULL, NULL);
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
        $$ = ast_make(AST_TRANSLATION_UNIT, NULL, NULL, NULL, NULL, NULL);
        smallcpp_ast = $$;
      }
    | translation_unit external_declaration {
        ast_add($1, $2);
        $$ = $1;
        smallcpp_ast = $$;
      }
    ;

external_declaration
    : function_definition { $$ = $1; }
    | constructor_definition { $$ = $1; }
    | destructor_definition { $$ = $1; }
    | declaration { $$ = $1; }
    | namespace_definition { $$ = $1; }
    | using_declaration { $$ = $1; }
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
        $$ = ast_make(AST_NAMESPACE, $2, NULL, NULL, NULL, NULL);
      }
    | NAMESPACE IDENTIFIER '{' external_declaration_list '}' {
        $$ = ast_make(AST_NAMESPACE, $2, $4, NULL, NULL, NULL);
      }
    ;

external_declaration_list
    : external_declaration { $$ = $1; }
    | external_declaration_list external_declaration { ast_append($1, $2); $$ = $1; }
    ;

function_definition
    : declaration_specifiers declarator compound_statement {
        $$ = ast_make(AST_METHOD, NULL, $1, $2, $3, NULL);
      }
    ;

declaration
    : declaration_specifiers init_declarator_list ';' {
        note_declarators($2);
        $$ = ast_make(AST_DECLARATION, NULL, $1, $2, NULL, NULL);
      }
    | declaration_specifiers ';' {
        $$ = ast_make(AST_DECLARATION, NULL, $1, NULL, NULL, NULL);
      }
    ;

declaration_specifiers
    : declaration_specifier { $$ = $1; }
    | declaration_specifiers declaration_specifier { ast_append($1, $2); $$ = $1; }
    ;

declaration_specifier
    : storage_class_specifier { $$ = $1; }
    | type_specifier { $$ = $1; }
    | type_qualifier { $$ = $1; }
    | function_specifier { $$ = $1; }
    ;

storage_class_specifier
    : TYPEDEF { $$ = ast_named(AST_SPECIFIER, "typedef"); }
    | EXTERN { $$ = ast_named(AST_SPECIFIER, "extern"); }
    | STATIC { $$ = ast_named(AST_SPECIFIER, "static"); }
    | AUTO { $$ = ast_named(AST_SPECIFIER, "auto"); }
    | REGISTER { $$ = ast_named(AST_SPECIFIER, "register"); }
    ;

type_specifier
    : VOID { $$ = ast_named(AST_SPECIFIER, "void"); }
    | CHAR { $$ = ast_named(AST_SPECIFIER, "char"); }
    | SHORT { $$ = ast_named(AST_SPECIFIER, "short"); }
    | INT { $$ = ast_named(AST_SPECIFIER, "int"); }
    | LONG { $$ = ast_named(AST_SPECIFIER, "long"); }
    | FLOAT { $$ = ast_named(AST_SPECIFIER, "float"); }
    | DOUBLE { $$ = ast_named(AST_SPECIFIER, "double"); }
    | SIGNED { $$ = ast_named(AST_SPECIFIER, "signed"); }
    | UNSIGNED { $$ = ast_named(AST_SPECIFIER, "unsigned"); }
    | BOOL { $$ = ast_named(AST_SPECIFIER, "bool"); }
    | struct_or_union_specifier { $$ = $1; }
    | enum_specifier { $$ = $1; }
    | class_specifier { $$ = $1; }
    | TYPEDEF_NAME { $$ = ast_text(AST_TYPE_NAME, $1); }
    ;

struct_or_union_specifier
    : struct_or_union IDENTIFIER '{' struct_declaration_list '}' {
        $$ = ast_make((AstKind) $1, $2, $4, NULL, NULL, NULL);
      }
    | struct_or_union '{' struct_declaration_list '}' {
        $$ = ast_make((AstKind) $1, NULL, $3, NULL, NULL, NULL);
      }
    | struct_or_union IDENTIFIER {
        $$ = ast_make((AstKind) $1, $2, NULL, NULL, NULL, NULL);
      }
    ;

struct_or_union
    : STRUCT { $$ = AST_STRUCT; }
    | UNION { $$ = AST_UNION; }
    ;

struct_declaration_list
    : struct_declaration { $$ = $1; }
    | struct_declaration_list struct_declaration { ast_append($1, $2); $$ = $1; }
    ;

/* Every member has a declarator, so the struct is non-empty and C99. */
struct_declaration
    : specifier_qualifier_list struct_declarator_list ';' {
        $$ = ast_make(AST_DECLARATION, NULL, $1, $2, NULL, NULL);
      }
    | error ';' { yyerrok; $$ = NULL; }
    ;

specifier_qualifier_list
    : specifier_qualifier { $$ = $1; }
    | specifier_qualifier_list specifier_qualifier { ast_append($1, $2); $$ = $1; }
    ;

specifier_qualifier
    : type_specifier { $$ = $1; }
    | type_qualifier { $$ = $1; }
    ;

struct_declarator_list
    : struct_declarator { $$ = $1; }
    | struct_declarator_list ',' struct_declarator { ast_append($1, $3); $$ = $1; }
    ;

/* A bit-field is a struct or union member. A ':' after a class name is
 * still the base-clause, and a ':' after a constructor is its initializer. */
struct_declarator
    : declarator { $$ = $1; }
    | declarator ':' constant_expression {
        $$ = ast_make(AST_BITFIELD, NULL, $1, $3, NULL, NULL);
      }
    ;

enum_specifier
    : ENUM '{' enumerator_list '}' { $$ = ast_make(AST_ENUM, NULL, $3, NULL, NULL, NULL); }
    | ENUM '{' enumerator_list ',' '}' { $$ = ast_make(AST_ENUM, NULL, $3, NULL, NULL, NULL); }
    | ENUM IDENTIFIER '{' enumerator_list '}' {
        $$ = ast_make(AST_ENUM, $2, $4, NULL, NULL, NULL);
      }
    | ENUM IDENTIFIER '{' enumerator_list ',' '}' {
        $$ = ast_make(AST_ENUM, $2, $4, NULL, NULL, NULL);
      }
    | ENUM IDENTIFIER { $$ = ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL); }
    ;

enumerator_list
    : enumerator { $$ = $1; }
    | enumerator_list ',' enumerator { ast_append($1, $3); $$ = $1; }
    ;

enumerator
    : IDENTIFIER { $$ = ast_text(AST_ENUMERATOR, $1); }
    | IDENTIFIER '=' constant_expression {
        $$ = ast_make(AST_ENUMERATOR, $1, $3, NULL, NULL, NULL);
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
 * ~Name(). Out of line they are Name::Name and Name::~Name.
 * Inside a class, the scanner returns the class name as CTOR_NAME when
 * it starts a member and the next token is '('. That keeps the
 * constructor distinct from a declaration that begins with a type name.
 */
class_specifier
    : CLASS name { smallcpp_push_class($2->text); } base_clause '{' class_member_list_opt '}' {
        smallcpp_pop_class();
        $$ = ast_make(AST_CLASS, NULL, $2, $4, $6, NULL);
      }
    | CLASS name { smallcpp_push_class($2->text); } '{' class_member_list_opt '}' {
        smallcpp_pop_class();
        $$ = ast_make(AST_CLASS, NULL, $2, NULL, $5, NULL);
      }
    | CLASS name { $$ = ast_make(AST_CLASS, NULL, $2, NULL, NULL, NULL); }
    | CLASS '{' class_member_list_opt '}' {
        $$ = ast_make(AST_CLASS, NULL, NULL, NULL, $3, NULL);
      }
    ;

name
    : IDENTIFIER { $$ = ast_text(AST_IDENT, $1); }
    | TYPEDEF_NAME { $$ = ast_text(AST_TYPE_NAME, $1); }
    ;

base_clause
    : ':' base_specifier_list { $$ = $2; }
    ;

base_specifier_list
    : base_specifier { $$ = $1; }
    | base_specifier_list ',' base_specifier { ast_append($1, $3); $$ = $1; }
    ;

base_specifier
    : access_keyword name { $$ = ast_make(AST_BASE, NULL, $1, $2, NULL, NULL); }
    | name { $$ = ast_make(AST_BASE, NULL, $1, NULL, NULL, NULL); }
    ;

access_keyword
    : PUBLIC { $$ = ast_named(AST_ACCESS, "public"); }
    | PRIVATE { $$ = ast_named(AST_ACCESS, "private"); }
    | PROTECTED { $$ = ast_named(AST_ACCESS, "protected"); }
    ;

class_member_list_opt
    : %empty { $$ = NULL; }
    | class_member_list { $$ = $1; }
    ;

class_member_list
    : class_member { $$ = $1; }
    | class_member_list class_member { ast_append($1, $2); $$ = $1; }
    ;

class_member
    : access_keyword ':' { $$ = $1; }
    | constructor_member { $$ = $1; }
    | destructor_member { $$ = $1; }
    | declaration { $$ = $1; }
    | function_definition { $$ = $1; }
    | using_declaration { $$ = $1; }
    | error ';' { yyerrok; $$ = NULL; }
    ;

constructor_member
    : CTOR_NAME '(' ')' ctor_initializer_opt compound_statement {
        $$ = ctor_spelling($1, NULL, $4, $5);
      }
    | CTOR_NAME '(' ')' ';' { $$ = ctor_spelling($1, NULL, NULL, NULL); }
    | CTOR_NAME '(' parameter_type_list ')' ctor_initializer_opt compound_statement {
        $$ = ctor_spelling($1, $3, $5, $6);
      }
    | CTOR_NAME '(' parameter_type_list ')' ';' { $$ = ctor_spelling($1, $3, NULL, NULL); }
    ;

destructor_member
    : '~' name '(' ')' compound_statement {
        if (!text_is($2, smallcpp_current_class()))
            yyerror("destructor name does not match the class");
        $$ = make_dtor($2, $5);
      }
    | '~' name '(' ')' ';' {
        if (!text_is($2, smallcpp_current_class()))
            yyerror("destructor name does not match the class");
        $$ = make_dtor($2, NULL);
      }
    ;

constructor_definition
    : name SCOPE name '(' ')' ctor_initializer_opt compound_statement {
        if (!names_equal($1, $3))
            yyerror("constructor name does not match the class");
        ast_free($1);
        $$ = make_ctor($3, NULL, $6, $7);
      }
    | name SCOPE name '(' parameter_type_list ')' ctor_initializer_opt compound_statement {
        if (!names_equal($1, $3))
            yyerror("constructor name does not match the class");
        ast_free($1);
        $$ = make_ctor($3, $5, $7, $8);
      }
    ;

destructor_definition
    : name SCOPE '~' name '(' ')' compound_statement {
        if (!names_equal($1, $4))
            yyerror("destructor name does not match the class");
        ast_free($1);
        $$ = make_dtor($4, $7);
      }
    | name SCOPE '~' name '(' ')' ';' {
        if (!names_equal($1, $4))
            yyerror("destructor name does not match the class");
        ast_free($1);
        $$ = make_dtor($4, NULL);
      }
    ;

ctor_initializer_opt
    : %empty { $$ = NULL; }
    | ':' mem_initializer_list { $$ = $2; }
    ;

mem_initializer_list
    : mem_initializer { $$ = $1; }
    | mem_initializer_list ',' mem_initializer { ast_append($1, $3); $$ = $1; }
    ;

mem_initializer
    : name '(' ')' { $$ = make_mem_init($1, NULL); }
    | name '(' argument_expression_list ')' { $$ = make_mem_init($1, $3); }
    ;

using_declaration
    : USING NAMESPACE name ';' {
        $$ = ast_make(AST_USING, strdup("namespace"), $3, NULL, NULL, NULL);
      }
    | USING using_name ';' { $$ = ast_make(AST_USING, NULL, $2, NULL, NULL, NULL); }
    ;

using_name
    : name { $$ = $1; }
    | nested_name { $$ = $1; }
    ;

type_qualifier
    : CONST { $$ = ast_named(AST_SPECIFIER, "const"); }
    | VOLATILE { $$ = ast_named(AST_SPECIFIER, "volatile"); }
    ;

function_specifier
    : INLINE { $$ = ast_named(AST_SPECIFIER, "inline"); }
    | VIRTUAL { $$ = ast_named(AST_SPECIFIER, "virtual"); }
    ;

init_declarator_list
    : init_declarator { $$ = $1; }
    | init_declarator_list ',' init_declarator { ast_append($1, $3); $$ = $1; }
    ;

init_declarator
    : declarator { $$ = $1; }
    | declarator '=' initializer { $$ = ast_make(AST_INIT, NULL, $1, $3, NULL, NULL); }
    ;

/* Brace initialization is written after '='. T x{...} is not supported.
 * .member = value and [index] = value are designated initializers. */
initializer
    : assignment_expression { $$ = $1; }
    | brace_initializer { $$ = $1; }
    ;

brace_initializer
    : '{' '}' { $$ = ast_make(AST_INIT, NULL, NULL, NULL, NULL, NULL); }
    | '{' initializer_list '}' { $$ = ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL); }
    | '{' initializer_list ',' '}' { $$ = ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL); }
    ;

initializer_list
    : initializer { $$ = $1; }
    | designator_list '=' initializer { $$ = designate($1, $3); }
    | initializer_list ',' initializer { ast_append($1, $3); $$ = $1; }
    | initializer_list ',' designator_list '=' initializer {
        ast_append($1, designate($3, $5));
        $$ = $1;
      }
    ;

designator_list
    : designator { $$ = $1; }
    | designator_list designator { ast_append($1, $2); $$ = $1; }
    ;

designator
    : '.' IDENTIFIER { $$ = ast_make(AST_DESIGNATED, $2, NULL, NULL, NULL, NULL); }
    | '[' constant_expression ']' { $$ = ast_make(AST_DESIGNATED, NULL, $2, NULL, NULL, NULL); }
    ;

/* Object and function declarators. No empty brackets, so no flexible array. */
declarator
    : pointer direct_declarator { $$ = ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL); }
    | direct_declarator { $$ = $1; }
    ;

direct_declarator
    : declarator_id { $$ = $1; }
    | '(' declarator ')' { $$ = $2; }
    | direct_declarator '[' constant_expression ']' {
        $$ = ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL);
      }
    | direct_declarator '(' ')' cv_qualifier_opt {
        $$ = ast_make(AST_FUNCTION_DECL, NULL, $1, NULL, $4, NULL);
      }
    | direct_declarator '(' parameter_type_list ')' cv_qualifier_opt {
        $$ = ast_make(AST_FUNCTION_DECL, NULL, $1, $3, $5, NULL);
      }
    ;

/* const and volatile after a parameter list. No ref-qualifiers. */
cv_qualifier_opt
    : %empty { $$ = NULL; }
    | cv_qualifier_seq { $$ = $1; }
    ;

cv_qualifier_seq
    : type_qualifier { $$ = $1; }
    | cv_qualifier_seq type_qualifier { ast_append($1, $2); $$ = $1; }
    ;

/* Point::move is qualified. The last component is an identifier, not a type. */
declarator_id
    : IDENTIFIER { $$ = ast_text(AST_IDENT, $1); }
    | name SCOPE declarator_id { $$ = ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL); }
    ;

pointer
    : '*' { $$ = ast_named(AST_POINTER, "*"); }
    | '*' type_qualifier_list { $$ = ast_make(AST_POINTER, NULL, $2, NULL, NULL, NULL); }
    | '*' pointer { $$ = ast_make(AST_POINTER, NULL, $2, NULL, NULL, NULL); }
    | '*' type_qualifier_list pointer { $$ = ast_make(AST_POINTER, NULL, $2, $3, NULL, NULL); }
    ;

type_qualifier_list
    : type_qualifier { $$ = $1; }
    | type_qualifier_list type_qualifier { ast_append($1, $2); $$ = $1; }
    ;

parameter_type_list
    : parameter_list { $$ = $1; }
    | parameter_list ',' ELLIPSIS {
        ast_append($1, ast_named(AST_ELLIPSIS, "..."));
        $$ = $1;
      }
    ;

parameter_list
    : parameter_declaration { $$ = $1; }
    | parameter_list ',' parameter_declaration { ast_append($1, $3); $$ = $1; }
    ;

parameter_declaration
    : parameter_specifiers { $$ = ast_make(AST_PARAMETER, NULL, $1, NULL, NULL, NULL); }
    | parameter_specifiers parameter_declarator {
        note_declarator($2);
        $$ = ast_make(AST_PARAMETER, NULL, $1, $2, NULL, NULL);
      }
    | parameter_specifiers parameter_declarator '=' assignment_expression {
        note_declarator($2);
        $$ = ast_make(AST_PARAMETER, NULL, $1, $2, $4, NULL);
      }
    ;

/* register is the only storage class both languages allow on a parameter. */
parameter_specifiers
    : parameter_specifier { $$ = $1; }
    | parameter_specifiers parameter_specifier { ast_append($1, $2); $$ = $1; }
    ;

parameter_specifier
    : type_specifier { $$ = $1; }
    | type_qualifier { $$ = $1; }
    | REGISTER { $$ = ast_named(AST_SPECIFIER, "register"); }
    ;

/* Named and abstract parameter declarators. Empty [] adjusts to a pointer. */
parameter_declarator
    : pointer direct_parameter_declarator {
        $$ = ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL);
      }
    | direct_parameter_declarator { $$ = $1; }
    | pointer { $$ = $1; }
    ;

direct_parameter_declarator
    : IDENTIFIER { $$ = ast_text(AST_IDENT, $1); }
    | '(' parameter_declarator ')' { $$ = $2; }
    | '[' ']' { $$ = ast_named(AST_ARRAY, "[]"); }
    | '[' constant_expression ']' { $$ = ast_make(AST_ARRAY, NULL, $2, NULL, NULL, NULL); }
    | '(' ')' { $$ = ast_make(AST_FUNCTION_DECL, NULL, NULL, NULL, NULL, NULL); }
    | '(' parameter_type_list ')' { $$ = ast_make(AST_FUNCTION_DECL, NULL, $2, NULL, NULL, NULL); }
    | direct_parameter_declarator '[' ']' {
        $$ = ast_make(AST_ARRAY, NULL, $1, NULL, NULL, NULL);
      }
    | direct_parameter_declarator '[' constant_expression ']' {
        $$ = ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL);
      }
    | direct_parameter_declarator '(' parameter_type_list ')' {
        $$ = ast_make(AST_FUNCTION_DECL, NULL, $1, $3, NULL, NULL);
      }
    ;

type_name
    : specifier_qualifier_list { $$ = $1; }
    | specifier_qualifier_list abstract_declarator {
        $$ = ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL);
      }
    ;

abstract_declarator
    : pointer { $$ = $1; }
    | direct_abstract_declarator { $$ = $1; }
    | pointer direct_abstract_declarator {
        $$ = ast_make(AST_DECLARATOR, NULL, $1, $2, NULL, NULL);
      }
    ;

direct_abstract_declarator
    : '(' abstract_declarator ')' { $$ = $2; }
    | '[' ']' { $$ = ast_named(AST_ARRAY, "[]"); }
    | '[' constant_expression ']' { $$ = ast_make(AST_ARRAY, NULL, $2, NULL, NULL, NULL); }
    | '(' ')' { $$ = ast_make(AST_FUNCTION_DECL, NULL, NULL, NULL, NULL, NULL); }
    | '(' parameter_type_list ')' { $$ = ast_make(AST_FUNCTION_DECL, NULL, $2, NULL, NULL, NULL); }
    | direct_abstract_declarator '[' ']' {
        $$ = ast_make(AST_ARRAY, NULL, $1, NULL, NULL, NULL);
      }
    | direct_abstract_declarator '[' constant_expression ']' {
        $$ = ast_make(AST_ARRAY, NULL, $1, $3, NULL, NULL);
      }
    | direct_abstract_declarator '(' parameter_type_list ')' {
        $$ = ast_make(AST_FUNCTION_DECL, NULL, $1, $3, NULL, NULL);
      }
    ;

statement
    : labeled_statement { $$ = $1; }
    | compound_statement { $$ = $1; }
    | expression_statement { $$ = $1; }
    | selection_statement { $$ = $1; }
    | iteration_statement { $$ = $1; }
    | jump_statement { $$ = $1; }
    | try_statement { $$ = $1; }
    | throw_statement { $$ = $1; }
    | error ';' { yyerrok; $$ = NULL; }
    ;

labeled_statement
    : IDENTIFIER ':' statement { $$ = ast_make(AST_LABEL, $1, $3, NULL, NULL, NULL); }
    | CASE constant_expression ':' statement {
        $$ = ast_make(AST_CASE, NULL, $2, $4, NULL, NULL);
      }
    | DEFAULT ':' statement { $$ = ast_make(AST_DEFAULT, NULL, $3, NULL, NULL, NULL); }
    ;

compound_statement
    : '{' '}' { $$ = ast_make(AST_COMPOUND, NULL, NULL, NULL, NULL, NULL); }
    | '{' block_item_list '}' { $$ = ast_make(AST_COMPOUND, NULL, $2, NULL, NULL, NULL); }
    ;

block_item_list
    : block_item { $$ = $1; }
    | block_item_list block_item { ast_append($1, $2); $$ = $1; }
    ;

block_item
    : declaration { $$ = $1; }
    | statement { $$ = $1; }
    | using_declaration { $$ = $1; }
    ;

expression_statement
    : ';' { $$ = ast_named(AST_EXPR_STMT, ";"); }
    | expression ';' { $$ = ast_make(AST_EXPR_STMT, NULL, $1, NULL, NULL, NULL); }
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE {
        $$ = ast_make(AST_IF, NULL, $3, $5, NULL, NULL);
      }
    | IF '(' expression ')' statement ELSE statement {
        $$ = ast_make(AST_IF, NULL, $3, $5, $7, NULL);
      }
    | SWITCH '(' expression ')' statement {
        $$ = ast_make(AST_SWITCH, NULL, $3, $5, NULL, NULL);
      }
    ;

iteration_statement
    : WHILE '(' expression ')' statement {
        $$ = ast_make(AST_WHILE, NULL, $3, $5, NULL, NULL);
      }
    | DO statement WHILE '(' expression ')' ';' {
        $$ = ast_make(AST_DO, NULL, $2, $5, NULL, NULL);
      }
    | FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement {
        $$ = ast_make(AST_FOR, NULL, $3, $5, $7, $9);
      }
    | FOR '(' declaration expression_opt ';' expression_opt ')' statement {
        $$ = ast_make(AST_FOR, NULL, $3, $4, $6, $8);
      }
    ;

jump_statement
    : GOTO IDENTIFIER ';' { $$ = ast_make(AST_GOTO, $2, NULL, NULL, NULL, NULL); }
    | CONTINUE ';' { $$ = ast_named(AST_CONTINUE, "continue"); }
    | BREAK ';' { $$ = ast_named(AST_BREAK, "break"); }
    | RETURN ';' { $$ = ast_named(AST_RETURN, "return"); }
    | RETURN expression ';' { $$ = ast_make(AST_RETURN, NULL, $2, NULL, NULL, NULL); }
    ;

/*
 * try compound-statement catch-clause+
 * A catch names one parameter, or it is catch (...), which catches every
 * type. The ... is not a parameter and is not the variadic-parameter marker.
 * throw expression; is the C++ throw statement.
 */
try_statement
    : TRY compound_statement catch_clause_list {
        $$ = ast_make(AST_TRY, NULL, $2, $3, NULL, NULL);
      }
    ;

catch_clause_list
    : catch_clause { $$ = $1; }
    | catch_clause_list catch_clause { ast_append($1, $2); $$ = $1; }
    ;

catch_clause
    : CATCH '(' parameter_declaration ')' compound_statement {
        $$ = ast_make(AST_CATCH, NULL, $3, $5, NULL, NULL);
      }
    | CATCH '(' ELLIPSIS ')' compound_statement { $$ = ast_catch_all($5); }
    ;

throw_statement
    : THROW assignment_expression ';' { $$ = ast_make(AST_THROW, NULL, $2, NULL, NULL, NULL); }
    | THROW ';' { $$ = ast_named(AST_THROW, "throw"); }
    ;

primary_expression
    : IDENTIFIER { $$ = ast_text(AST_IDENT, $1); }
    | nested_name { $$ = $1; }
    | THIS { $$ = ast_named(AST_THIS, "this"); }
    | TRUE { $$ = ast_named(AST_LITERAL, "true"); }
    | FALSE { $$ = ast_named(AST_LITERAL, "false"); }
    | NULLPTR { $$ = ast_null("nullptr"); }
    | NULL_KEYWORD { $$ = ast_null("NULL"); }
    | CONSTANT { $$ = ast_text(AST_LITERAL, $1); }
    | STRING_LITERAL { $$ = ast_text(AST_LITERAL, $1); }
    | '(' expression ')' { $$ = $2; }
    | format_expression { $$ = $1; }
    ;

/*
 * print and format are native. The first operand is a string literal.
 * Replacement fields use C++23 syntax: {}, {0}, {:.2f}, and the other
 * standard format specs. The arguments follow the string.
 */
format_expression
    : PRINT '(' STRING_LITERAL ')' { $$ = native_format(AST_PRINT, $3, NULL); }
    | PRINT '(' STRING_LITERAL ',' argument_expression_list ')' {
        $$ = native_format(AST_PRINT, $3, $5);
      }
    | FORMAT '(' STRING_LITERAL ')' { $$ = native_format(AST_FORMAT, $3, NULL); }
    | FORMAT '(' STRING_LITERAL ',' argument_expression_list ')' {
        $$ = native_format(AST_FORMAT, $3, $5);
      }
    ;

/* A nested name has at least one '::', so a bare type name is not an expression. */
nested_name
    : name SCOPE name { $$ = ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL); }
    | nested_name SCOPE name { $$ = ast_make(AST_QUALIFIED, NULL, $1, $3, NULL, NULL); }
    ;

postfix_expression
    : primary_expression { $$ = $1; }
    | postfix_expression '[' expression ']' {
        $$ = ast_make(AST_INDEX, NULL, $1, $3, NULL, NULL);
      }
    | postfix_expression '(' ')' { $$ = ast_make(AST_CALL, NULL, $1, NULL, NULL, NULL); }
    | postfix_expression '(' argument_expression_list ')' {
        $$ = ast_make(AST_CALL, NULL, $1, $3, NULL, NULL);
      }
    | postfix_expression '.' IDENTIFIER { $$ = ast_make(AST_MEMBER, $3, $1, NULL, NULL, NULL); }
    | postfix_expression '.' '~' name '(' ')' { $$ = dtor_call($1, $4, 0); }
    | postfix_expression ARROW IDENTIFIER { $$ = ast_make(AST_ARROW, $3, $1, NULL, NULL, NULL); }
    | postfix_expression ARROW '~' name '(' ')' { $$ = dtor_call($1, $4, 1); }
    | postfix_expression INC { $$ = ast_unary("post++", $1); }
    | postfix_expression DEC { $$ = ast_unary("post--", $1); }
    ;

argument_expression_list
    : assignment_expression { $$ = $1; }
    | argument_expression_list ',' assignment_expression { ast_append($1, $3); $$ = $1; }
    ;

unary_expression
    : postfix_expression { $$ = $1; }
    | INC unary_expression { $$ = ast_unary("++", $2); }
    | DEC unary_expression { $$ = ast_unary("--", $2); }
    | unary_operator cast_expression { $$ = ast_unary($1, $2); }
    | SIZEOF unary_expression { $$ = ast_make(AST_SIZEOF, NULL, $2, NULL, NULL, NULL); }
    | SIZEOF '(' type_name ')' { $$ = ast_make(AST_SIZEOF, NULL, $3, NULL, NULL, NULL); }
    | NEW new_type %prec NEW_BARE { $$ = make_new($2, NULL, NULL, NULL); }
    | NEW new_type new_pointer { $$ = make_new($2, $3, NULL, NULL); }
    | NEW new_type new_arrays { $$ = make_new($2, NULL, $3, NULL); }
    | NEW new_type new_pointer new_arrays { $$ = make_new($2, $3, $4, NULL); }
    | NEW new_type '(' ')' { $$ = make_new($2, NULL, NULL, NULL); }
    | NEW new_type '(' argument_expression_list ')' { $$ = make_new($2, NULL, NULL, $4); }
    | NEW new_type new_pointer '(' ')' { $$ = make_new($2, $3, NULL, NULL); }
    | NEW new_type new_pointer '(' argument_expression_list ')' {
        $$ = make_new($2, $3, NULL, $5);
      }
    | DELETE cast_expression {
        if (!operand_is_pointer($2))
            yyerror("delete operand is not a pointer");
        $$ = ast_delete_noop($2);
      }
    ;

/*
 * new takes a class or typedef name, or one type specifier, then an
 * optional pointer declarator and an optional array bound. '*' and '['
 * belong to the new-expression. (new int) * p is multiplication.
 */
new_type
    : name { $$ = $1; }
    | VOID { $$ = ast_named(AST_SPECIFIER, "void"); }
    | CHAR { $$ = ast_named(AST_SPECIFIER, "char"); }
    | SHORT { $$ = ast_named(AST_SPECIFIER, "short"); }
    | INT { $$ = ast_named(AST_SPECIFIER, "int"); }
    | LONG { $$ = ast_named(AST_SPECIFIER, "long"); }
    | FLOAT { $$ = ast_named(AST_SPECIFIER, "float"); }
    | DOUBLE { $$ = ast_named(AST_SPECIFIER, "double"); }
    | SIGNED { $$ = ast_named(AST_SPECIFIER, "signed"); }
    | UNSIGNED { $$ = ast_named(AST_SPECIFIER, "unsigned"); }
    | BOOL { $$ = ast_named(AST_SPECIFIER, "bool"); }
    | STRUCT IDENTIFIER { $$ = ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL); }
    | STRUCT TYPEDEF_NAME { $$ = ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL); }
    | ENUM IDENTIFIER { $$ = ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL); }
    | ENUM TYPEDEF_NAME { $$ = ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL); }
    ;

new_pointer
    : '*' { $$ = ast_named(AST_POINTER, "*"); }
    | '*' new_pointer { $$ = ast_make(AST_POINTER, strdup("*"), $2, NULL, NULL, NULL); }
    ;

new_arrays
    : '[' expression ']' { $$ = ast_make(AST_ARRAY, NULL, NULL, $2, NULL, NULL); }
    | '[' expression ']' new_arrays {
        $$ = ast_make(AST_ARRAY, NULL, $4, $2, NULL, NULL);
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
    : unary_expression { $$ = $1; }
    | '(' type_name ')' cast_expression { $$ = ast_make(AST_CAST, NULL, $2, $4, NULL, NULL); }
    ;

multiplicative_expression
    : cast_expression { $$ = $1; }
    | multiplicative_expression '*' cast_expression { $$ = ast_binary($1, "*", $3); }
    | multiplicative_expression '/' cast_expression { $$ = ast_binary($1, "/", $3); }
    | multiplicative_expression '%' cast_expression { $$ = ast_binary($1, "%", $3); }
    ;

additive_expression
    : multiplicative_expression { $$ = $1; }
    | additive_expression '+' multiplicative_expression { $$ = ast_binary($1, "+", $3); }
    | additive_expression '-' multiplicative_expression { $$ = ast_binary($1, "-", $3); }
    ;

shift_expression
    : additive_expression { $$ = $1; }
    | shift_expression SHL additive_expression { $$ = ast_binary($1, "<<", $3); }
    | shift_expression SHR additive_expression { $$ = ast_binary($1, ">>", $3); }
    ;

relational_expression
    : shift_expression { $$ = $1; }
    | relational_expression '<' shift_expression { $$ = ast_binary($1, "<", $3); }
    | relational_expression '>' shift_expression { $$ = ast_binary($1, ">", $3); }
    | relational_expression LE shift_expression { $$ = ast_binary($1, "<=", $3); }
    | relational_expression GE shift_expression { $$ = ast_binary($1, ">=", $3); }
    ;

equality_expression
    : relational_expression { $$ = $1; }
    | equality_expression EQEQ relational_expression { $$ = ast_binary($1, "==", $3); }
    | equality_expression NOTEQ relational_expression { $$ = ast_binary($1, "!=", $3); }
    ;

and_expression
    : equality_expression { $$ = $1; }
    | and_expression '&' equality_expression { $$ = ast_binary($1, "&", $3); }
    ;

exclusive_or_expression
    : and_expression { $$ = $1; }
    | exclusive_or_expression '^' and_expression { $$ = ast_binary($1, "^", $3); }
    ;

inclusive_or_expression
    : exclusive_or_expression { $$ = $1; }
    | inclusive_or_expression '|' exclusive_or_expression { $$ = ast_binary($1, "|", $3); }
    ;

logical_and_expression
    : inclusive_or_expression { $$ = $1; }
    | logical_and_expression ANDAND inclusive_or_expression { $$ = ast_binary($1, "&&", $3); }
    ;

logical_or_expression
    : logical_and_expression { $$ = $1; }
    | logical_or_expression OROR logical_and_expression { $$ = ast_binary($1, "||", $3); }
    ;

conditional_expression
    : logical_or_expression { $$ = $1; }
    | logical_or_expression '?' expression ':' conditional_expression {
        $$ = ast_make(AST_CONDITIONAL, NULL, $1, $3, $5, NULL);
      }
    ;

assignment_expression
    : conditional_expression { $$ = $1; }
    | unary_expression assignment_operator assignment_expression {
        $$ = ast_assign($1, $2, $3);
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
    : assignment_expression { $$ = $1; }
    | expression ',' assignment_expression { $$ = ast_binary($1, ",", $3); }
    ;

expression_opt
    : %empty { $$ = NULL; }
    | expression { $$ = $1; }
    ;

constant_expression
    : conditional_expression { $$ = $1; }
    ;

%%

static int
yyreport_syntax_error(const yypcontext_t *ctx)
{
    YYLTYPE *loc = yypcontext_location(ctx);
    yysymbol_kind_t token = yypcontext_token(ctx);
    char message[160];

    if (loc != NULL)
        yylloc = *loc;
    if (token == YYSYMBOL_YYEMPTY)
        yyerror("syntax error");
    else {
        snprintf(message, sizeof message, "syntax error, unexpected %s",
                 yysymbol_name(token));
        yyerror(message);
    }
    return 0;
}
