/* libmain - flex run-time support library "main" function */

/* $NetBSD: libmain.c,v 1.6 1999/03/06 00:19:07 mycroft Exp $ */

#include <sys/cdefs.h>

int yylex __P((void));
int main __P((int, char **, char **));

/*ARGSUSED*/
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
