/*	$NetBSD: error.y,v 1.1.1.4.2.1 2014/05/22 15:44:17 yamt Exp $	*/

%{
int yylex(void);
static void yyerror(const char *);
%}
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
yylex(void)
{
    return -1;
}

static void
yyerror(const char* s)
{
    printf("%s\n", s);
}
