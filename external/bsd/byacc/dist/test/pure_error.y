/*	$NetBSD: pure_error.y,v 1.1.1.1 2010/12/23 23:36:27 christos Exp $	*/

%%
S: error
%%

#include <stdio.h>

int
main(void)
{
    printf("yyparse() = %d\n", yyparse());
    return 0;
}

int
yylex(YYSTYPE *value)
{
    return value ? 0 : -1;
}

static void
yyerror(const char* s)
{
    printf("%s\n", s);
}
