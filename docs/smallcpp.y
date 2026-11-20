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
 * A class definition goes in a header whose name ends in `.shpp`. The
 * method bodies go in a `.scpp` file that is compiled and linked.
 * Include the `.shpp` file. Do not include a `.scpp` file. A method
 * written inside the class body is inline and stays in the header.
 * Ordinary preprocessing is already done. This file shows that syntax.
 * It has no embedded code and no actions. The compiler grammar, whose
 * actions build the syntax tree, is src/compiler/smallcpp.y. The rules
 * below are those rules.
 *
 * A syntax error prints the file, line, and column, the source line, and
 * the tokens that were expected. There is no caret under the line. The
 * error token then resynchronizes at the next semicolon so later errors
 * are still reported. That recovery is not part of the language.
 *
 * Deliberate omissions:
 * - variable-length arrays, including '[*]' and 'static' inside '[]'
 * - restrict
 * - flexible array members. Empty '[]' is recognized only on parameters,
 * where both languages adjust the parameter to a pointer
 * - compound literals. (type){ ... } does not parse, including (int[]){1, 2}
 * - C++11 brace initialization without '='. A brace list is written
 * after '='. references, rvalue references, auto as a type, lambdas,
 * constexpr, trailing return types, and ref-qualifiers. auto is only
 * the C storage-class specifier
 * - _Bool, _Complex, and _Imaginary, which are not C++11. wchar_t is
 * not a keyword
 * - K&R definitions that name parameters before the body. () has no
 * parameters
 * - empty structs, anonymous struct members, anonymous union members,
 * and unnamed bit-fields. A bit-field names its member
 * - u, U, and u8 string prefixes, which are not C99
 * - anonymous namespaces. A namespace has a name
 * - super. A base method is Base::method(), or method() when the name
 * is not hidden. this remains a keyword
 * - templates and operator overloading
 * - an array bound is syntactically an expression. A bound that is not an
 * integer constant expression is a VLA and is excluded
 */

%token IDENTIFIER TYPEDEF_NAME CONSTANT STRING_LITERAL CTOR_NAME
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

%start translation_unit

%%

/* Start symbol. A translation unit is zero or more external declarations. */
translation_unit
    : %empty
    | translation_unit external_declaration
    ;

external_declaration
    : function_definition
    | constructor_definition
    | destructor_definition
    | declaration
    | namespace_definition
    | using_declaration
    | error ';'
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
    | BOOL
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
    | error ';'
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

/* A bit-field is a struct or union member. A ':' after a class name is
 * still the base-clause, and a ':' after a constructor is its initializer. */
struct_declarator
    : declarator
    | declarator ':' constant_expression
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
    : ':' base_specifier_list
    ;

base_specifier_list
    : base_specifier
    | base_specifier_list ',' base_specifier
    ;

base_specifier
    : access_keyword name
    | name
    | VIRTUAL access_keyword name
    | access_keyword VIRTUAL name
    | VIRTUAL name
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
    | constructor_member
    | destructor_member
    | declaration
    | function_definition
    | using_declaration
    | error ';'
    ;

constructor_member
    : CTOR_NAME '(' ')' ctor_initializer_opt compound_statement
    | CTOR_NAME '(' ')' ';'
    | CTOR_NAME '(' parameter_type_list ')' ctor_initializer_opt compound_statement
    | CTOR_NAME '(' parameter_type_list ')' ';'
    ;

destructor_params
    : '(' ')'
    | '(' parameter_type_list ')'
    ;

destructor_member
    : '~' name destructor_params compound_statement
    | '~' name destructor_params ';'
    | VIRTUAL '~' name destructor_params compound_statement
    | VIRTUAL '~' name destructor_params ';'
    ;

constructor_definition
    : name SCOPE name '(' ')' ctor_initializer_opt compound_statement
    | name SCOPE name '(' parameter_type_list ')' ctor_initializer_opt compound_statement
    ;

destructor_definition
    : name SCOPE '~' name destructor_params compound_statement
    | name SCOPE '~' name destructor_params ';'
    ;

ctor_initializer_opt
    : %empty
    | ':' mem_initializer_list
    ;

mem_initializer_list
    : mem_initializer
    | mem_initializer_list ',' mem_initializer
    ;

mem_initializer
    : name '(' ')'
    | name '(' argument_expression_list ')'
    ;

using_declaration
    : USING NAMESPACE name ';'
    | USING using_name ';'
    ;

using_name
    : name
    | nested_name
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

/* Brace initialization is written after '='. T x{...} is not supported.
 * .member = value and [index] = value are designated initializers. */
initializer
    : assignment_expression
    | brace_initializer
    ;

brace_initializer
    : '{' '}'
    | '{' initializer_list '}'
    | '{' initializer_list ',' '}'
    ;

initializer_list
    : initializer
    | designator_list '=' initializer
    | initializer_list ',' initializer
    | initializer_list ',' designator_list '=' initializer
    ;

designator_list
    : designator
    | designator_list designator
    ;

designator
    : '.' IDENTIFIER
    | '[' constant_expression ']'
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
    | direct_declarator '(' ')' cv_qualifier_opt pure_specifier_opt
    | direct_declarator '(' parameter_type_list ')' cv_qualifier_opt pure_specifier_opt
    ;

/* A pure virtual method is = 0. Any other constant is rejected. */
pure_specifier_opt
    : %empty %prec LOWER_THAN_PURE
    | '=' CONSTANT
    ;

/* const and volatile after a parameter list. No ref-qualifiers. */
cv_qualifier_opt
    : %empty
    | cv_qualifier_seq
    ;

cv_qualifier_seq
    : type_qualifier
    | cv_qualifier_seq type_qualifier
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
    | parameter_specifiers parameter_declarator '=' assignment_expression
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
    | '(' ')'
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
    | '(' ')'
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
    | error ';'
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
    | using_declaration
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
 * try compound-statement catch-clause+
 * A catch names one parameter, or it is catch (...), which catches every
 * type. The ... is not a parameter and is not the variadic-parameter marker.
 * throw expression; is the C++ throw statement.
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
    | CATCH '(' ELLIPSIS ')' compound_statement
    ;

throw_statement
    : THROW assignment_expression ';'
    | THROW ';'
    ;

primary_expression
    : IDENTIFIER
    | nested_name
    | THIS
    | TRUE
    | FALSE
    | NULLPTR
    | NULL_KEYWORD
    | CONSTANT
    | STRING_LITERAL
    | '(' expression ')'
    | format_expression
    ;

/*
 * print and format are native. The first operand is a string literal.
 * Replacement fields use C++23 syntax: {}, {0}, {:.2f}, and the other
 * standard format specs. The arguments follow the string.
 */
format_expression
    : PRINT '(' STRING_LITERAL ')'
    | PRINT '(' STRING_LITERAL ',' argument_expression_list ')'
    | FORMAT '(' STRING_LITERAL ')'
    | FORMAT '(' STRING_LITERAL ',' argument_expression_list ')'
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
    | postfix_expression '.' '~' name '(' ')'
    | postfix_expression '.' '~' name '(' argument_expression_list ')'
    | postfix_expression ARROW IDENTIFIER
    | postfix_expression ARROW '~' name '(' ')'
    | postfix_expression ARROW '~' name '(' argument_expression_list ')'
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
    | TYPEID '(' expression ')'
    | TYPEID '(' type_name ')'
    | NEW new_type %prec NEW_BARE
    | NEW new_type new_pointer
    | NEW new_type new_arrays
    | NEW new_type new_pointer new_arrays
    | NEW new_type '(' ')'
    | NEW new_type '(' argument_expression_list ')'
    | NEW new_type new_pointer '(' ')'
    | NEW new_type new_pointer '(' argument_expression_list ')'
    | DELETE cast_expression
    ;

/*
 * new takes a class or typedef name, or one type specifier, then an
 * optional pointer declarator and an optional array bound. '*' and '['
 * belong to the new-expression. (new int) * p is multiplication.
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
    | BOOL
    | STRUCT IDENTIFIER
    | STRUCT TYPEDEF_NAME
    | ENUM IDENTIFIER
    | ENUM TYPEDEF_NAME
    ;

new_pointer
    : '*'
    | '*' new_pointer
    ;

new_arrays
    : '[' expression ']'
    | '[' expression ']' new_arrays
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
