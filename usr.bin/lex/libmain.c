/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/usr.bin/lex/Attic/libmain.c,v 1.2 1995/05/05 05:35:31 jtc Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	return yylex();
	}
