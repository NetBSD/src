/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/gnu/usr.bin/lex/lib/Attic/libmain.c,v 1.5 1993/12/06 19:26:04 jtc Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	return yylex();
	}
