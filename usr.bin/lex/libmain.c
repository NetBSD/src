/* libmain - flex run-time support library "main" function */

/* $Header: /cvsroot/src/usr.bin/lex/Attic/libmain.c,v 1.3 1995/06/05 19:44:52 pk Exp $ */

#include <sys/cdefs.h>

int yylex __P((void));
int main __P((int, char **, char **));

int
main( argc, argv, envp )
int argc;
char *argv[];
char *envp[];
	{
	return yylex();
	}
