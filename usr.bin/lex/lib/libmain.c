/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/usr.bin/lex/lib/Attic/libmain.c,v 1.4 1993/12/02 19:14:31 jtc Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	return yylex();
	}
