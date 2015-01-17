/*	$NetBSD: null_lex.l,v 1.1.1.2 2015/01/17 16:34:17 christos Exp $	*/

%{
#include "null_gram.h"

void null_error(const char *fmt, ...) {}
int yywrap(void) { return 0; }
%}
%option nounput
%option noinput
%%
