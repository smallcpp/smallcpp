/*
 * Scanner for the smallcpp grammar in smallcpp.y.
 * Keywords are the plain C++ spellings. None of them has an sc_ prefix.
 * The identifier after class is recorded so a later use is TYPEDEF_NAME.
 * A typedef name is recorded the same way: the identifier outside
 * parentheses, up to the semicolon.
 * An invalid character is reported with its file, line, column, and
 * source line, and then skipped, so scanning continues.
 * "..." is one token. A parameter list uses it as a variadic parameter.
 * catch (...) uses the same token, and the grammar does not make it a
 * parameter.
 * nullptr and NULL are keywords for the same null pointer constant.
 * delete is a keyword. It is not an identifier.
 * super is not a keyword. using introduces a using-declaration or a
 * using-directive. using is not a macro. this remains a keyword.
 * bool, true, and false are keywords.
 */
%option noyywrap
%option nodefault
%option yylineno
%x BLOCK_COMMENT

%{
#include "smallcpp.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void yyerror(const char *message);

int yycolumn = 1;

#define YY_USER_ACTION update_location();

static void update_location(void)
{
    int i;
    int newlines = 0;
    int line;

    for (i = 0; i < yyleng; i++) {
        if (yytext[i] == '\n')
            newlines++;
    }
    line = yylineno - newlines;
    if (line < 1)
        line = 1;
    yylloc.first_line = line;
    yylloc.first_column = yycolumn;
    yylloc.last_line = line;
    yylloc.last_column = yycolumn > 0 ? yycolumn : 1;
    for (i = 0; i < yyleng; i++) {
        if (yytext[i] == '\n') {
            line++;
            yycolumn = 1;
        } else {
            yylloc.last_line = line;
            yylloc.last_column = yycolumn;
            yycolumn++;
        }
    }
}

struct keyword {
    const char *name;
    int token;
};

#define TYPE_MAX 256

static char *type_names[TYPE_MAX];
static int type_count;
static int class_name_pending;
static int typedef_pending;
static int paren_depth;
static int comment_line;
static int comment_column;
static int member_start;

#define CLASS_MAX 32

static const char *class_stack[CLASS_MAX];
static int class_depth;

void smallcpp_push_class(const char *name)
{
    if (name == NULL || class_depth >= CLASS_MAX)
        return;
    class_stack[class_depth++] = name;
}

void smallcpp_pop_class(void)
{
    if (class_depth > 0)
        class_depth--;
}

const char *smallcpp_current_class(void)
{
    if (class_depth == 0)
        return NULL;
    return class_stack[class_depth - 1];
}

static int at_constructor_name(const char *text);

static int keyword_cmp(const void *key, const void *item)
{
    const struct keyword *kw = item;
    return strcmp(key, kw->name);
}

static int is_type_name(const char *name)
{
    int i;

    for (i = 0; i < type_count; i++) {
        if (strcmp(type_names[i], name) == 0)
            return 1;
    }
    return 0;
}

static void note_class_name(const char *name)
{
    if (type_count >= TYPE_MAX || is_type_name(name))
        return;
    type_names[type_count] = strdup(name);
    if (type_names[type_count] != NULL)
        type_count++;
}

static int keyword_token(const char *name)
{
    static const struct keyword keywords[] = {
        {"NULL", NULL_KEYWORD},
        {"auto", AUTO},
        {"bool", BOOL},
        {"break", BREAK},
        {"case", CASE},
        {"catch", CATCH},
        {"char", CHAR},
        {"class", CLASS},
        {"const", CONST},
        {"continue", CONTINUE},
        {"default", DEFAULT},
        {"delete", DELETE},
        {"do", DO},
        {"double", DOUBLE},
        {"else", ELSE},
        {"enum", ENUM},
        {"extern", EXTERN},
        {"false", FALSE},
        {"float", FLOAT},
        {"for", FOR},
        {"format", FORMAT},
        {"goto", GOTO},
        {"if", IF},
        {"inline", INLINE},
        {"int", INT},
        {"long", LONG},
        {"namespace", NAMESPACE},
        {"new", NEW},
        {"nullptr", NULLPTR},
        {"print", PRINT},
        {"private", PRIVATE},
        {"protected", PROTECTED},
        {"public", PUBLIC},
        {"register", REGISTER},
        {"return", RETURN},
        {"short", SHORT},
        {"signed", SIGNED},
        {"sizeof", SIZEOF},
        {"static", STATIC},
        {"struct", STRUCT},
        {"switch", SWITCH},
        {"this", THIS},
        {"throw", THROW},
        {"true", TRUE},
        {"try", TRY},
        {"typedef", TYPEDEF},
        {"typeid", TYPEID},
        {"union", UNION},
        {"unsigned", UNSIGNED},
        {"using", USING},
        {"virtual", VIRTUAL},
        {"void", VOID},
        {"volatile", VOLATILE},
        {"while", WHILE},
    };
    const struct keyword *found;

    found = bsearch(name, keywords, sizeof keywords / sizeof keywords[0],
                    sizeof keywords[0], keyword_cmp);
    if (found != NULL)
        return found->token;
    return IDENTIFIER;
}

static int finish(int token, const char *text)
{
    if (token == '(')
        paren_depth++;
    else if (token == ')' && paren_depth > 0)
        paren_depth--;
    if (class_name_pending && text != NULL
        && (token == IDENTIFIER || token == TYPEDEF_NAME))
        note_class_name(text);
    if (typedef_pending && paren_depth == 0 && text != NULL
        && (token == IDENTIFIER || token == TYPEDEF_NAME))
        note_class_name(text);
    if (token == ';')
        typedef_pending = 0;
    if (token == TYPEDEF)
        typedef_pending = 1;
    class_name_pending = (token == CLASS);
    if (token == IDENTIFIER || token == TYPEDEF_NAME || token == CTOR_NAME
        || token == CONSTANT || token == STRING_LITERAL) {
        const char *spelling = text != NULL ? text : yytext;
        yylval.text = strdup(spelling);
    }
    member_start = (token == '{' || token == '}' || token == ';' || token == ':');
    return token;
}

static void lexical_error(const char *message)
{
    class_name_pending = 0;
    yyerror(message);
}
%}

%%

[ \t\r\n]+              ;
"//"[^\n]*              ;
"/*"                    {
                            comment_line = yylloc.first_line;
                            comment_column = yylloc.first_column;
                            BEGIN(BLOCK_COMMENT);
                        }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n       ;
<BLOCK_COMMENT>.        ;
<BLOCK_COMMENT><<EOF>>  {
                            yylloc.first_line = comment_line;
                            yylloc.last_line = comment_line;
                            yylloc.first_column = comment_column;
                            yylloc.last_column = comment_column;
                            lexical_error("unterminated comment");
                            return 0;
                        }

"..."                   { return finish(ELLIPSIS, NULL); }
"<<="                   { return finish(SHL_ASSIGN, NULL); }
">>="                   { return finish(SHR_ASSIGN, NULL); }
"::"                    { return finish(SCOPE, NULL); }
"->"                    { return finish(ARROW, NULL); }
"++"                    { return finish(INC, NULL); }
"--"                    { return finish(DEC, NULL); }
"<<"                    { return finish(SHL, NULL); }
">>"                    { return finish(SHR, NULL); }
"=="                    { return finish(EQEQ, NULL); }
"!="                    { return finish(NOTEQ, NULL); }
"<="                    { return finish(LE, NULL); }
">="                    { return finish(GE, NULL); }
"&&"                    { return finish(ANDAND, NULL); }
"||"                    { return finish(OROR, NULL); }
"+="                    { return finish(ADD_ASSIGN, NULL); }
"-="                    { return finish(SUB_ASSIGN, NULL); }
"*="                    { return finish(MUL_ASSIGN, NULL); }
"/="                    { return finish(DIV_ASSIGN, NULL); }
"%="                    { return finish(MOD_ASSIGN, NULL); }
"&="                    { return finish(AND_ASSIGN, NULL); }
"|="                    { return finish(OR_ASSIGN, NULL); }
"^="                    { return finish(XOR_ASSIGN, NULL); }

L?\"([^\"\\\n]|\\.)*\"  { return finish(STRING_LITERAL, NULL); }
\"([^\"\\\n]|\\.)*      { lexical_error("unterminated string"); }
L?\'([^\'\\\n]|\\.)+\'  { return finish(CONSTANT, NULL); }
\'([^\'\\\n]|\\.)*      { lexical_error("unterminated character constant"); }

0[xX][0-9A-Fa-f]+([uU][lL]?|[lL][uU]?|[lL][lL][uU]?)? { return finish(CONSTANT, NULL); }
[0-9]+\.[0-9]*([eE][+-]?[0-9]+)?[fFlL]? { return finish(CONSTANT, NULL); }
\.[0-9]+([eE][+-]?[0-9]+)?[fFlL]? { return finish(CONSTANT, NULL); }
[0-9]+([eE][+-]?[0-9]+)?([uU][lL]?|[lL][uU]?|[lL][lL][uU]?|[fF])? { return finish(CONSTANT, NULL); }

[A-Za-z_][A-Za-z0-9_]*  {
                            int length = yyleng;
                            char *spelling = strndup(yytext, (size_t) length);
                            int token = keyword_token(spelling != NULL ? spelling : yytext);
                            if (token == IDENTIFIER && at_constructor_name(spelling))
                                token = CTOR_NAME;
                            else if (token == IDENTIFIER && is_type_name(spelling))
                                token = TYPEDEF_NAME;
                            token = finish(token, spelling != NULL ? spelling : yytext);
                            free(spelling);
                            return token;
                        }

[(){}\[\];,:?~!+\-*/%&|^.<>=] { return finish(yytext[0], NULL); }

.                       {
                            unsigned char c = (unsigned char) yytext[0];
                            char message[64];

                            if (c >= 32 && c < 127)
                                snprintf(message, sizeof message,
                                         "invalid character '%c'", c);
                            else
                                snprintf(message, sizeof message,
                                         "invalid character 0x%02x", c);
                            lexical_error(message);
                        }

%%

static int peek_lparen(void)
{
    int i = 0;
    int c = 0;
    char buf[64];

    while (i < (int) sizeof buf) {
        c = input();
        if (c == 0 || c == EOF)
            break;
        buf[i++] = (char) c;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            continue;
        break;
    }
    while (i > 0)
        unput(buf[--i]);
    return c == '(';
}

static int at_constructor_name(const char *text)
{
    if (!member_start || class_depth == 0 || text == NULL)
        return 0;
    return strcmp(text, class_stack[class_depth - 1]) == 0 && peek_lparen();
}
