/* libyywrap - flex run-time support library "yywrap" function */

/* $NetBSD: libyywrap.c,v 1.5 1998/01/05 05:15:54 perry Exp $ */

#include <sys/cdefs.h>

int yywrap __P((void));

int
yywrap()
	{
	return 1;
	}
