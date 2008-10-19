/*	$NetBSD: null_lex.l,v 1.1.1.1.4.2 2008/10/19 22:39:40 haad Exp $	*/

%{
#include "null_gram.h"

void yyerror(const char *fmt, ...) {}
int yywrap(void) { return 0; }
%}
%%
