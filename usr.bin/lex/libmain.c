/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/usr.bin/lex/Attic/libmain.c,v 1.1.1.1 1996/12/10 06:06:48 mikel Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	while ( yylex() != 0 )
		;

	return 0;
	}
