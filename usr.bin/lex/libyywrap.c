/* libyywrap - flex run-time support library "yywrap" function */

/* $Header: /cvsroot/src/usr.bin/lex/Attic/libyywrap.c,v 1.3 1995/06/05 19:44:55 pk Exp $ */

#include <sys/cdefs.h>

int yywrap __P((void));

int yywrap()
	{
	return 1;
	}
