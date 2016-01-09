/*	$NetBSD: err_syntax17.y,v 1.1.1.3 2016/01/09 21:59:45 christos Exp $	*/

%{
int yylex(void);
static void yyerror(const char *);
%}

%%

S: { error
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
