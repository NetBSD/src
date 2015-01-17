/*	$NetBSD: null_gram.y,v 1.1.1.2 2015/01/17 16:34:17 christos Exp $	*/

%{
void yyerror(const char *fmt, ...);
extern int yylex(void);
%}

%%

token:
