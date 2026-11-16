/*
 * smallcpp grammar. Start symbol: translation_unit.
 *
 * The base language is the syntax shared by ISO C99 and ISO C++11, so a
 * program that stays in that subset is acceptable to a modern C compiler
 * and a modern C++ compiler. smallcpp adds classes, single inheritance,
 * exceptions, namespaces, and precise garbage collection. It does not add
 * templates, multiple inheritance, operator overloading, or the C++
 * standard library.
 *
 * No keyword has an sc_ prefix. class, namespace, public, private,
 * protected, virtual, try, catch, throw, new, and this are the plain C++
 * keywords, not macros. namespace uses C++ syntax: namespace name { ... }.
 * A qualified name uses '::'. Allocation is a new-expression. There is no
 * delete spelling; precise collection is a compiler and runtime
 * requirement, and the compiler records the type in each new-expression.
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
 *   - designated initializers
 *   - compound literals, which are C99 and not C++11
 *   - C++11 brace initialization, references, rvalue references, auto as a
 *     type, lambdas, constexpr, nullptr, and trailing return types. auto is
 *     only the C storage-class specifier
 *   - _Bool, _Complex, and _Imaginary, which are not C++11. bool, true,
 *     false, and wchar_t are not keywords here; they are not C99 keywords
 *   - K&R definitions and a parameter list written (). Write (void). The
 *     two languages disagree about what () means
 *   - empty structs, anonymous struct members, and anonymous union members
 *   - u, U, and u8 string prefixes, which are not C99
 *   - a second base. The base-clause is a single name, so a comma does not
 *     parse. Access on that base is optional: class D : public B
 *   - templates and operator overloading
 *   - bit-fields. A ':' after a class name introduces the base-clause
 *   - an array bound is syntactically an expression. A bound that is not an
 *     integer constant expression is a VLA and is excluded
 *   - a new-expression names a class, typedef, or plain type specifier.
 *     It does not include a pointer or array declarator
 */

%{
#include "ast.h"

#include <stdlib.h>

int yylex(void);
void yyerror(const char *message);
%}

%union {
    char *text;
    struct AstNode *node;
    int kind;
}

%token <text> IDENTIFIER TYPEDEF_NAME CONSTANT STRING_LITERAL
%token CLASS "class" NAMESPACE "namespace"
%token PUBLIC "public" PRIVATE "private" PROTECTED "protected"
%token VIRTUAL "virtual" THIS "this"
%token TRY "try" CATCH "catch" THROW "throw" NEW "new"
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
%type <node> primary_expression nested_name postfix_expression
%type <node> argument_expression_list unary_expression new_type cast_expression
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
    | declaration { $$ = $1; }
    | namespace_definition { $$ = $1; }
    | error ';' { yyerrok; }
    ;

/*
 * namespace name { ... }
 * The same syntax as C++. Nested namespaces are allowed.
 * A namespace is not a statement and cannot appear inside a function.
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
    | error ';' { yyerrok; }
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

/* No bit-fields. A ':' after a class name is the base-clause. */
struct_declarator
    : declarator { $$ = $1; }
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
 * class Name : access base { members }
 * One base. A second base, written with a comma, does not parse.
 * public:, private:, and protected: are the C++ access specifiers.
 * The scanner records the class name so it can later be a type name.
 */
class_specifier
    : CLASS name base_clause '{' class_member_list_opt '}' {
        $$ = ast_make(AST_CLASS, NULL, $2, $3, $5, NULL);
      }
    | CLASS name '{' class_member_list_opt '}' {
        $$ = ast_make(AST_CLASS, NULL, $2, NULL, $4, NULL);
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
    : ':' name { $$ = ast_make(AST_BASE, NULL, $2, NULL, NULL, NULL); }
    | ':' access_keyword name { $$ = ast_make(AST_BASE, NULL, $2, $3, NULL, NULL); }
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
    | declaration { $$ = $1; }
    | function_definition { $$ = $1; }
    | error ';' { yyerrok; }
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

/* No designators and no brace-initialization without '='. */
initializer
    : assignment_expression { $$ = $1; }
    | '{' initializer_list '}' { $$ = ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL); }
    | '{' initializer_list ',' '}' { $$ = ast_make(AST_INIT, NULL, $2, NULL, NULL, NULL); }
    ;

initializer_list
    : initializer { $$ = $1; }
    | initializer_list ',' initializer { ast_append($1, $3); $$ = $1; }
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
    | direct_declarator '(' parameter_type_list ')' {
        $$ = ast_make(AST_FUNCTION_DECL, NULL, $1, $3, NULL, NULL);
      }
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
        $$ = ast_make(AST_PARAMETER, NULL, $1, $2, NULL, NULL);
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
    | error ';' { yyerrok; }
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
 * try compound-statement catch ( parameter-declaration ) compound-statement
 * One or more catches. throw expression; is the C++ throw statement.
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
    ;

throw_statement
    : THROW assignment_expression ';' { $$ = ast_make(AST_THROW, NULL, $2, NULL, NULL, NULL); }
    | THROW ';' { $$ = ast_named(AST_THROW, "throw"); }
    ;

primary_expression
    : IDENTIFIER { $$ = ast_text(AST_IDENT, $1); }
    | nested_name { $$ = $1; }
    | THIS { $$ = ast_named(AST_THIS, "this"); }
    | CONSTANT { $$ = ast_text(AST_LITERAL, $1); }
    | STRING_LITERAL { $$ = ast_text(AST_LITERAL, $1); }
    | '(' expression ')' { $$ = $2; }
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
    | postfix_expression ARROW IDENTIFIER { $$ = ast_make(AST_ARROW, $3, $1, NULL, NULL, NULL); }
    | postfix_expression INC { $$ = ast_unary("++", $1); }
    | postfix_expression DEC { $$ = ast_unary("--", $1); }
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
    | NEW new_type { $$ = ast_make(AST_NEW, NULL, $2, NULL, NULL, NULL); }
    | NEW new_type '(' ')' { $$ = ast_make(AST_NEW, NULL, $2, NULL, NULL, NULL); }
    | NEW new_type '(' argument_expression_list ')' {
        $$ = ast_make(AST_NEW, NULL, $2, $4, NULL, NULL);
      }
    ;

/*
 * new takes a class or typedef name, or one type specifier.
 * Pointer and array types are not written in the new-expression, so '*'
 * stays the multiplication operator.
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
    | STRUCT IDENTIFIER { $$ = ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL); }
    | STRUCT TYPEDEF_NAME { $$ = ast_make(AST_STRUCT, $2, NULL, NULL, NULL, NULL); }
    | ENUM IDENTIFIER { $$ = ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL); }
    | ENUM TYPEDEF_NAME { $$ = ast_make(AST_ENUM, $2, NULL, NULL, NULL, NULL); }
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
