/*
 * smallcpp grammar. Start symbol: translation_unit.
 *
 * The base language is the syntax shared by ISO C99 and ISO C++11. smallcpp
 * adds classes, single inheritance, exceptions, namespaces, and precise
 * garbage collection. It does not add templates, multiple inheritance,
 * operator overloading, or the C++ standard library.
 *
 * Source text is acceptable to any ISO C compiler from C99 on and any C++
 * compiler from C++11 on. C++ keywords and the '::' token are not used.
 * Those tokens are not shared: class, namespace, try, catch, throw, new,
 * public, private, protected, virtual, and '::' are illegal in C99, and
 * this, new, and class are not identifiers in C++. The spellings below are
 * ordinary identifiers in both languages. A C99 or C++11 compiler is given
 * the compatibility macros before the program and therefore expands them.
 * A smallcpp parser does not expand them; it recognizes the tokens below.
 * Ordinary preprocessing (# directives) is already done. This file is the
 * grammar only. It is not a compiler.
 *
 *   void *sc_alloc(unsigned long size);
 *   int sc_ex_push(void);
 *   int sc_ex_catch(void);
 *   int sc_ex_once(void);
 *   void sc_ex_throw(void);
 *
 *   #define sc_class struct
 *   #define sc_public
 *   #define sc_private
 *   #define sc_protected
 *   #define sc_base(base) struct base sc_base_member
 *   #define sc_method
 *   #define sc_virtual
 *   #define sc_namespace(name)
 *   #define sc_end_namespace
 *   #define sc_qual(ns, name) ns##_##name
 *   #define sc_try if (sc_ex_push())
 *   #define sc_catch(declaration) else if (sc_ex_catch()) for (declaration; sc_ex_once();)
 *   #define sc_throw(value) do { (void)(value); sc_ex_throw(); } while (0)
 *   #define sc_new(type) ((type*)sc_alloc(sizeof(type)))
 *
 * Precise collection is a compiler and runtime requirement. sc_new is the
 * only added allocation spelling. The compiler records the type argument
 * and treats pointer variables as roots. There is no delete spelling.
 *
 * Deliberate omissions, beyond the absent C++ features above:
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
 *   - member functions defined or declared inside a class. A function
 *     declaration inside a struct is not C99. Methods are file-scope or
 *     namespace-scope functions marked sc_method. The receiver is an
 *     ordinary parameter; this is not a keyword
 *   - a second sc_base. Inheritance is one base subobject, written before
 *     the data members. The macro expands to the member sc_base_member,
 *     not to a C++ base. The member name is an ordinary identifier
 *   - '::'. sc_namespace groups declarations and expands to nothing under
 *     C and C++. sc_qual(ns, name) is the qualified name both accept; the
 *     macro pastes ns_name
 *   - access markers expand to nothing. smallcpp enforces them; a C or C++
 *     compiler does not
 *   - the try/catch macros expand to if / else if, and the catch declaration
 *     is the init of a for statement, which is what puts the name in scope
 *     for a C or C++ compiler. That expansion is not the exception
 *     semantics. The declaration must be legal as a for-init and must be
 *     one macro argument, so a comma has to sit inside parentheses. A const
 *     object needs an initializer; this grammar does not give the catch
 *     parameter one. The catch declarator always has a name
 *   - an array bound is syntactically an expression. A bound that is not an
 *     integer constant expression is a VLA and is excluded
 */

%{
int yylex(void);
void yyerror(const char *message);
%}

%token IDENTIFIER TYPEDEF_NAME CONSTANT STRING_LITERAL
%token SC_CLASS "sc_class"
%token SC_BASE "sc_base"
%token SC_PUBLIC "sc_public"
%token SC_PRIVATE "sc_private"
%token SC_PROTECTED "sc_protected"
%token SC_METHOD "sc_method"
%token SC_VIRTUAL "sc_virtual"
%token SC_NAMESPACE "sc_namespace"
%token SC_END_NAMESPACE "sc_end_namespace"
%token SC_QUAL "sc_qual"
%token SC_TRY "sc_try"
%token SC_CATCH "sc_catch"
%token SC_THROW "sc_throw"
%token SC_NEW "sc_new"
%token AUTO BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE ENUM
%token EXTERN FLOAT FOR GOTO IF INLINE INT LONG REGISTER RETURN SHORT
%token SIGNED SIZEOF STATIC STRUCT SWITCH TYPEDEF UNION UNSIGNED VOID
%token VOLATILE WHILE
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
 * sc_namespace ( name ) ... sc_end_namespace
 * expands to the declarations alone. Nested namespaces are allowed.
 * A namespace is not a statement and cannot appear inside a function.
 */
namespace_definition
    : SC_NAMESPACE '(' IDENTIFIER ')' SC_END_NAMESPACE
    | SC_NAMESPACE '(' IDENTIFIER ')' external_declaration_list SC_END_NAMESPACE
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
    | SC_METHOD
    | SC_VIRTUAL
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

struct_declarator
    : declarator
    | declarator ':' constant_expression
    | ':' constant_expression
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
 * sc_class Name { access-markers, at most one leading sc_base, members }
 * A class with neither a base nor a data member is rejected: the macro
 * expands to a struct, and an empty struct is not C99.
 * sc_base, when present, is the first member after any leading access
 * markers, so the base subobject is first. A second sc_base does not parse.
 */
class_specifier
    : SC_CLASS IDENTIFIER '{' class_member_list '}'
    | SC_CLASS '{' class_member_list '}'
    | SC_CLASS IDENTIFIER
    ;

class_member_list
    : access_run class_after_access
    ;

access_run
    : %empty
    | access_run access_marker
    ;

class_after_access
    : SC_BASE '(' IDENTIFIER ')' ';' class_rest
    | struct_declaration class_rest
    ;

class_rest
    : %empty
    | class_rest access_marker
    | class_rest struct_declaration
    ;

access_marker
    : SC_PUBLIC
    | SC_PRIVATE
    | SC_PROTECTED
    ;

type_qualifier
    : CONST
    | VOLATILE
    ;

function_specifier
    : INLINE
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
    : IDENTIFIER
    | SC_QUAL '(' IDENTIFIER ',' IDENTIFIER ')'
    | '(' declarator ')'
    | direct_declarator '[' constant_expression ']'
    | direct_declarator '(' parameter_type_list ')'
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
    | SC_QUAL '(' IDENTIFIER ',' IDENTIFIER ')'
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
 * sc_try compound-statement
 *     sc_catch ( specifiers named-declarator ) compound-statement
 * One or more catches. The declarator has a name so the for-init macro
 * can declare it. sc_catch ( int e ) is one macro argument.
 */
try_statement
    : SC_TRY compound_statement catch_clause_list
    ;

catch_clause_list
    : catch_clause
    | catch_clause_list catch_clause
    ;

catch_clause
    : SC_CATCH '(' parameter_specifiers declarator ')' compound_statement
    ;

throw_statement
    : SC_THROW '(' assignment_expression ')' ';'
    ;

primary_expression
    : IDENTIFIER
    | CONSTANT
    | STRING_LITERAL
    | '(' expression ')'
    | SC_QUAL '(' IDENTIFIER ',' IDENTIFIER ')'
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
    | SC_NEW '(' type_name ')'
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
