/*
 * Scanner for the smallcpp grammar in smallcpp.y.
 * Keywords are the plain C++ spellings. None of them has an sc_ prefix.
 * The identifier after class is recorded so a later use is TYPEDEF_NAME.
 * Other typedef names are not recorded.
 */
%option noyywrap
%option nounput
%option noinput
%option nodefault
%option yylineno
%x BLOCK_COMMENT

%{
#include "smallcpp.tab.h"

#include <stdlib.h>
#include <string.h>

struct keyword {
    const char *name;
    int token;
};

#define TYPE_MAX 256

static char *type_names[TYPE_MAX];
static int type_count;
static int class_name_pending;

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
        {"auto", AUTO},
        {"break", BREAK},
        {"case", CASE},
        {"catch", CATCH},
        {"char", CHAR},
        {"class", CLASS},
        {"const", CONST},
        {"continue", CONTINUE},
        {"default", DEFAULT},
        {"do", DO},
        {"double", DOUBLE},
        {"else", ELSE},
        {"enum", ENUM},
        {"extern", EXTERN},
        {"float", FLOAT},
        {"for", FOR},
        {"goto", GOTO},
        {"if", IF},
        {"inline", INLINE},
        {"int", INT},
        {"long", LONG},
        {"namespace", NAMESPACE},
        {"new", NEW},
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
        {"try", TRY},
        {"typedef", TYPEDEF},
        {"union", UNION},
        {"unsigned", UNSIGNED},
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
    if (class_name_pending && text != NULL
        && (token == IDENTIFIER || token == TYPEDEF_NAME))
        note_class_name(text);
    class_name_pending = (token == CLASS);
    return token;
}
%}

%%

[ \t\r\n]+              ;
"//"[^\n]*              ;
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n       ;
<BLOCK_COMMENT>.        ;

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
L?\'([^\'\\\n]|\\.)+\'  { return finish(CONSTANT, NULL); }

0[xX][0-9A-Fa-f]+([uU][lL]?|[lL][uU]?|[lL][lL][uU]?)? { return finish(CONSTANT, NULL); }
[0-9]+\.[0-9]*([eE][+-]?[0-9]+)?[fFlL]? { return finish(CONSTANT, NULL); }
\.[0-9]+([eE][+-]?[0-9]+)?[fFlL]? { return finish(CONSTANT, NULL); }
[0-9]+([eE][+-]?[0-9]+)?([uU][lL]?|[lL][uU]?|[lL][lL][uU]?|[fF])? { return finish(CONSTANT, NULL); }

[A-Za-z_][A-Za-z0-9_]*  {
                            int token = keyword_token(yytext);
                            if (token == IDENTIFIER && is_type_name(yytext))
                                token = TYPEDEF_NAME;
                            return finish(token, yytext);
                        }

.                       { return finish(yytext[0], NULL); }

%%
