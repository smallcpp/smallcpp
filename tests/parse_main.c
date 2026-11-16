#include <stdio.h>
#include <stdlib.h>

extern int yyparse(void);
extern FILE *yyin;
extern int yylineno;

static const char *input_path;

void yyerror(const char *message)
{
    fprintf(stderr, "%s:%d: %s\n", input_path, yylineno, message);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 2;
    }
    input_path = argv[1];
    yyin = fopen(argv[1], "r");
    if (yyin == NULL) {
        perror(argv[1]);
        return 2;
    }
    if (yyparse() != 0) {
        fclose(yyin);
        return 1;
    }
    fclose(yyin);
    return 0;
}
