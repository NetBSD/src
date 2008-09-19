/*	$NetBSD: null_lex.l,v 1.1.1.1 2008/09/19 20:07:21 christos Exp $	*/

%{
#include "null_gram.h"

void yyerror(const char *fmt, ...) {}
int yywrap(void) { return 0; }
%}
%%
