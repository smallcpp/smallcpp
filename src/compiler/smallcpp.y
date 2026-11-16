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
 * done. This file is the grammar only. It is not a compiler.
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
int yylex(void);
void yyerror(const char *message);
%}

%token IDENTIFIER TYPEDEF_NAME CONSTANT STRING_LITERAL
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

%start translation_unit

%%

/* Start symbol. A translation unit is zero or more external declarations. */
translation_unit
    : %empty
    | translation_unit external_declaration
    ;

external_declaration
    : function_definition
    | declaration
    | namespace_definition
    ;

/*
 * namespace name { ... }
 * The same syntax as C++. Nested namespaces are allowed.
 * A namespace is not a statement and cannot appear inside a function.
 */
namespace_definition
    : NAMESPACE IDENTIFIER '{' '}'
    | NAMESPACE IDENTIFIER '{' external_declaration_list '}'
    ;

external_declaration_list
    : external_declaration
    | external_declaration_list external_declaration
    ;

function_definition
    : declaration_specifiers declarator compound_statement
    ;

declaration
    : declaration_specifiers init_declarator_list ';'
    | declaration_specifiers ';'
    ;

declaration_specifiers
    : declaration_specifier
    | declaration_specifiers declaration_specifier
    ;

declaration_specifier
    : storage_class_specifier
    | type_specifier
    | type_qualifier
    | function_specifier
    ;

storage_class_specifier
    : TYPEDEF
    | EXTERN
    | STATIC
    | AUTO
    | REGISTER
    ;

type_specifier
    : VOID
    | CHAR
    | SHORT
    | INT
    | LONG
    | FLOAT
    | DOUBLE
    | SIGNED
    | UNSIGNED
    | struct_or_union_specifier
    | enum_specifier
    | class_specifier
    | TYPEDEF_NAME
    ;

struct_or_union_specifier
    : struct_or_union IDENTIFIER '{' struct_declaration_list '}'
    | struct_or_union '{' struct_declaration_list '}'
    | struct_or_union IDENTIFIER
    ;

struct_or_union
    : STRUCT
    | UNION
    ;

struct_declaration_list
    : struct_declaration
    | struct_declaration_list struct_declaration
    ;

/* Every member has a declarator, so the struct is non-empty and C99. */
struct_declaration
    : specifier_qualifier_list struct_declarator_list ';'
    ;

specifier_qualifier_list
    : specifier_qualifier
    | specifier_qualifier_list specifier_qualifier
    ;

specifier_qualifier
    : type_specifier
    | type_qualifier
    ;

struct_declarator_list
    : struct_declarator
    | struct_declarator_list ',' struct_declarator
    ;

/* No bit-fields. A ':' after a class name is the base-clause. */
struct_declarator
    : declarator
    ;

enum_specifier
    : ENUM '{' enumerator_list '}'
    | ENUM '{' enumerator_list ',' '}'
    | ENUM IDENTIFIER '{' enumerator_list '}'
    | ENUM IDENTIFIER '{' enumerator_list ',' '}'
    | ENUM IDENTIFIER
    ;

enumerator_list
    : enumerator
    | enumerator_list ',' enumerator
    ;

enumerator
    : IDENTIFIER
    | IDENTIFIER '=' constant_expression
    ;

/*
 * class Name : access base { members }
 * One base. A second base, written with a comma, does not parse.
 * public:, private:, and protected: are the C++ access specifiers.
 * The scanner records the class name so it can later be a type name.
 */
class_specifier
    : CLASS name base_clause '{' class_member_list_opt '}'
    | CLASS name '{' class_member_list_opt '}'
    | CLASS name
    | CLASS '{' class_member_list_opt '}'
    ;

name
    : IDENTIFIER
    | TYPEDEF_NAME
    ;

base_clause
    : ':' name
    | ':' access_keyword name
    ;

access_keyword
    : PUBLIC
    | PRIVATE
    | PROTECTED
    ;

class_member_list_opt
    : %empty
    | class_member_list
    ;

class_member_list
    : class_member
    | class_member_list class_member
    ;

class_member
    : access_keyword ':'
    | declaration
    | function_definition
    ;

type_qualifier
    : CONST
    | VOLATILE
    ;

function_specifier
    : INLINE
    | VIRTUAL
    ;

init_declarator_list
    : init_declarator
    | init_declarator_list ',' init_declarator
    ;

init_declarator
    : declarator
    | declarator '=' initializer
    ;

/* No designators and no brace-initialization without '='. */
initializer
    : assignment_expression
    | '{' initializer_list '}'
    | '{' initializer_list ',' '}'
    ;

initializer_list
    : initializer
    | initializer_list ',' initializer
    ;

/* Object and function declarators. No empty brackets, so no flexible array. */
declarator
    : pointer direct_declarator
    | direct_declarator
    ;

direct_declarator
    : declarator_id
    | '(' declarator ')'
    | direct_declarator '[' constant_expression ']'
    | direct_declarator '(' parameter_type_list ')'
    ;

/* Point::move is qualified. The last component is an identifier, not a type. */
declarator_id
    : IDENTIFIER
    | name SCOPE declarator_id
    ;

pointer
    : '*'
    | '*' type_qualifier_list
    | '*' pointer
    | '*' type_qualifier_list pointer
    ;

type_qualifier_list
    : type_qualifier
    | type_qualifier_list type_qualifier
    ;

parameter_type_list
    : parameter_list
    | parameter_list ',' ELLIPSIS
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

parameter_declaration
    : parameter_specifiers
    | parameter_specifiers parameter_declarator
    ;

/* register is the only storage class both languages allow on a parameter. */
parameter_specifiers
    : parameter_specifier
    | parameter_specifiers parameter_specifier
    ;

parameter_specifier
    : type_specifier
    | type_qualifier
    | REGISTER
    ;

/* Named and abstract parameter declarators. Empty [] adjusts to a pointer. */
parameter_declarator
    : pointer direct_parameter_declarator
    | direct_parameter_declarator
    | pointer
    ;

direct_parameter_declarator
    : IDENTIFIER
    | '(' parameter_declarator ')'
    | '[' ']'
    | '[' constant_expression ']'
    | '(' parameter_type_list ')'
    | direct_parameter_declarator '[' ']'
    | direct_parameter_declarator '[' constant_expression ']'
    | direct_parameter_declarator '(' parameter_type_list ')'
    ;

type_name
    : specifier_qualifier_list
    | specifier_qualifier_list abstract_declarator
    ;

abstract_declarator
    : pointer
    | direct_abstract_declarator
    | pointer direct_abstract_declarator
    ;

direct_abstract_declarator
    : '(' abstract_declarator ')'
    | '[' ']'
    | '[' constant_expression ']'
    | '(' parameter_type_list ')'
    | direct_abstract_declarator '[' ']'
    | direct_abstract_declarator '[' constant_expression ']'
    | direct_abstract_declarator '(' parameter_type_list ')'
    ;

statement
    : labeled_statement
    | compound_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | try_statement
    | throw_statement
    ;

labeled_statement
    : IDENTIFIER ':' statement
    | CASE constant_expression ':' statement
    | DEFAULT ':' statement
    ;

compound_statement
    : '{' '}'
    | '{' block_item_list '}'
    ;

block_item_list
    : block_item
    | block_item_list block_item
    ;

block_item
    : declaration
    | statement
    ;

expression_statement
    : ';'
    | expression ';'
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE
    | IF '(' expression ')' statement ELSE statement
    | SWITCH '(' expression ')' statement
    ;

iteration_statement
    : WHILE '(' expression ')' statement
    | DO statement WHILE '(' expression ')' ';'
    | FOR '(' expression_opt ';' expression_opt ';' expression_opt ')' statement
    | FOR '(' declaration expression_opt ';' expression_opt ')' statement
    ;

jump_statement
    : GOTO IDENTIFIER ';'
    | CONTINUE ';'
    | BREAK ';'
    | RETURN ';'
    | RETURN expression ';'
    ;

/*
 * try compound-statement catch ( parameter-declaration ) compound-statement
 * One or more catches. throw expression; is the C++ throw statement.
 */
try_statement
    : TRY compound_statement catch_clause_list
    ;

catch_clause_list
    : catch_clause
    | catch_clause_list catch_clause
    ;

catch_clause
    : CATCH '(' parameter_declaration ')' compound_statement
    ;

throw_statement
    : THROW assignment_expression ';'
    | THROW ';'
    ;

primary_expression
    : IDENTIFIER
    | nested_name
    | THIS
    | CONSTANT
    | STRING_LITERAL
    | '(' expression ')'
    ;

/* A nested name has at least one '::', so a bare type name is not an expression. */
nested_name
    : name SCOPE name
    | nested_name SCOPE name
    ;

postfix_expression
    : primary_expression
    | postfix_expression '[' expression ']'
    | postfix_expression '(' ')'
    | postfix_expression '(' argument_expression_list ')'
    | postfix_expression '.' IDENTIFIER
    | postfix_expression ARROW IDENTIFIER
    | postfix_expression INC
    | postfix_expression DEC
    ;

argument_expression_list
    : assignment_expression
    | argument_expression_list ',' assignment_expression
    ;

unary_expression
    : postfix_expression
    | INC unary_expression
    | DEC unary_expression
    | unary_operator cast_expression
    | SIZEOF unary_expression
    | SIZEOF '(' type_name ')'
    | NEW new_type
    | NEW new_type '(' ')'
    | NEW new_type '(' argument_expression_list ')'
    ;

/*
 * new takes a class or typedef name, or one type specifier.
 * Pointer and array types are not written in the new-expression, so '*'
 * stays the multiplication operator.
 */
new_type
    : name
    | VOID
    | CHAR
    | SHORT
    | INT
    | LONG
    | FLOAT
    | DOUBLE
    | SIGNED
    | UNSIGNED
    | STRUCT IDENTIFIER
    | STRUCT TYPEDEF_NAME
    | ENUM IDENTIFIER
    | ENUM TYPEDEF_NAME
    ;

unary_operator
    : '&'
    | '*'
    | '+'
    | '-'
    | '~'
    | '!'
    ;

cast_expression
    : unary_expression
    | '(' type_name ')' cast_expression
    ;

multiplicative_expression
    : cast_expression
    | multiplicative_expression '*' cast_expression
    | multiplicative_expression '/' cast_expression
    | multiplicative_expression '%' cast_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

shift_expression
    : additive_expression
    | shift_expression SHL additive_expression
    | shift_expression SHR additive_expression
    ;

relational_expression
    : shift_expression
    | relational_expression '<' shift_expression
    | relational_expression '>' shift_expression
    | relational_expression LE shift_expression
    | relational_expression GE shift_expression
    ;

equality_expression
    : relational_expression
    | equality_expression EQEQ relational_expression
    | equality_expression NOTEQ relational_expression
    ;

and_expression
    : equality_expression
    | and_expression '&' equality_expression
    ;

exclusive_or_expression
    : and_expression
    | exclusive_or_expression '^' and_expression
    ;

inclusive_or_expression
    : exclusive_or_expression
    | inclusive_or_expression '|' exclusive_or_expression
    ;

logical_and_expression
    : inclusive_or_expression
    | logical_and_expression ANDAND inclusive_or_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OROR logical_and_expression
    ;

conditional_expression
    : logical_or_expression
    | logical_or_expression '?' expression ':' conditional_expression
    ;

assignment_expression
    : conditional_expression
    | unary_expression assignment_operator assignment_expression
    ;

assignment_operator
    : '='
    | ADD_ASSIGN
    | SUB_ASSIGN
    | MUL_ASSIGN
    | DIV_ASSIGN
    | MOD_ASSIGN
    | SHL_ASSIGN
    | SHR_ASSIGN
    | AND_ASSIGN
    | XOR_ASSIGN
    | OR_ASSIGN
    ;

expression
    : assignment_expression
    | expression ',' assignment_expression
    ;

expression_opt
    : %empty
    | expression
    ;

constant_expression
    : conditional_expression
    ;

%%
