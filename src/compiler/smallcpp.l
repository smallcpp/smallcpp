/*
 * Scanner for the smallcpp grammar in smallcpp.y.
 * Keywords are the plain C++ spellings. None of them has an sc_ prefix.
 * The identifier after class is recorded so a later use is TYPEDEF_NAME.
 * Other typedef names are not recorded.
 * An invalid character is reported with its line and column and then
 * skipped, so scanning continues.
 */
%option noyywrap
%option nounput
%option noinput
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
static int comment_line;
static int comment_column;

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
    if (token == IDENTIFIER || token == TYPEDEF_NAME
        || token == CONSTANT || token == STRING_LITERAL) {
        const char *spelling = text != NULL ? text : yytext;
        yylval.text = strdup(spelling);
    }
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
                            int token = keyword_token(yytext);
                            if (token == IDENTIFIER && is_type_name(yytext))
                                token = TYPEDEF_NAME;
                            return finish(token, yytext);
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
