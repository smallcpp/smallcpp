/*
 * Scanner for the smallcpp grammar in smallcpp.y.
 * sc_* spellings are tokens. They are not expanded and they are not
 * identifiers. TYPEDEF_NAME is a grammar token, but this scanner returns
 * IDENTIFIER for every non-keyword name: the grammar has no action that
 * records typedefs.
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

static int keyword_cmp(const void *key, const void *item)
{
    const struct keyword *kw = item;
    return strcmp(key, kw->name);
}

static int keyword_token(const char *name)
{
    static const struct keyword keywords[] = {
        {"auto", AUTO},
        {"break", BREAK},
        {"case", CASE},
        {"char", CHAR},
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
        {"register", REGISTER},
        {"return", RETURN},
        {"sc_base", SC_BASE},
        {"sc_catch", SC_CATCH},
        {"sc_class", SC_CLASS},
        {"sc_end_namespace", SC_END_NAMESPACE},
        {"sc_method", SC_METHOD},
        {"sc_namespace", SC_NAMESPACE},
        {"sc_new", SC_NEW},
        {"sc_private", SC_PRIVATE},
        {"sc_protected", SC_PROTECTED},
        {"sc_public", SC_PUBLIC},
        {"sc_qual", SC_QUAL},
        {"sc_throw", SC_THROW},
        {"sc_try", SC_TRY},
        {"sc_virtual", SC_VIRTUAL},
        {"short", SHORT},
        {"signed", SIGNED},
        {"sizeof", SIZEOF},
        {"static", STATIC},
        {"struct", STRUCT},
        {"switch", SWITCH},
        {"typedef", TYPEDEF},
        {"union", UNION},
        {"unsigned", UNSIGNED},
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
%}

%%

[ \t\r\n]+              ;
"//"[^\n]*              ;
"/*"                    { BEGIN(BLOCK_COMMENT); }
<BLOCK_COMMENT>"*/"     { BEGIN(INITIAL); }
<BLOCK_COMMENT>\n       ;
<BLOCK_COMMENT>.        ;

"..."                   { return ELLIPSIS; }
"<<="                   { return SHL_ASSIGN; }
">>="                   { return SHR_ASSIGN; }
"->"                    { return ARROW; }
"++"                    { return INC; }
"--"                    { return DEC; }
"<<"                    { return SHL; }
">>"                    { return SHR; }
"=="                    { return EQEQ; }
"!="                    { return NOTEQ; }
"<="                    { return LE; }
">="                    { return GE; }
"&&"                    { return ANDAND; }
"||"                    { return OROR; }
"+="                    { return ADD_ASSIGN; }
"-="                    { return SUB_ASSIGN; }
"*="                    { return MUL_ASSIGN; }
"/="                    { return DIV_ASSIGN; }
"%="                    { return MOD_ASSIGN; }
"&="                    { return AND_ASSIGN; }
"|="                    { return OR_ASSIGN; }
"^="                    { return XOR_ASSIGN; }

L?\"([^\"\\\n]|\\.)*\"  { return STRING_LITERAL; }
L?\'([^\'\\\n]|\\.)+\'  { return CONSTANT; }

0[xX][0-9A-Fa-f]+([uU][lL]?|[lL][uU]?|[lL][lL][uU]?)? { return CONSTANT; }
[0-9]+\.[0-9]*([eE][+-]?[0-9]+)?[fFlL]? { return CONSTANT; }
\.[0-9]+([eE][+-]?[0-9]+)?[fFlL]? { return CONSTANT; }
[0-9]+([eE][+-]?[0-9]+)?([uU][lL]?|[lL][uU]?|[lL][lL][uU]?|[fF])? { return CONSTANT; }

[A-Za-z_][A-Za-z0-9_]*  { return keyword_token(yytext); }

.                       { return yytext[0]; }

%%
