/* libyywrap - flex run-time support library "yywrap" function */

/* $Header: /cvsroot/src/usr.bin/lex/Attic/libyywrap.c,v 1.4 1996/12/10 07:18:48 mikel Exp $ */

#include <sys/cdefs.h>

int yywrap __P((void));

int
yywrap()
	{
	return 1;
	}
