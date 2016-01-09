/*	$NetBSD: err_syntax26.y,v 1.1.1.3 2016/01/09 21:59:45 christos Exp $	*/

%{
int yylex(void);
static void yyerror(const char *);
%}

%type <tag2
