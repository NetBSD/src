/* libmain - flex run-time support library "main" function */

/* $NetBSD: libmain.c,v 1.5 1998/01/05 05:15:53 perry Exp $ */

#include <sys/cdefs.h>

int yylex __P((void));
int main __P((int, char **, char **));

int
main( argc, argv, envp )
int argc;
char *argv[];
char *envp[];
	{
	while ( yylex() != 0 )
		;

	return 0;
	}
