/* libyywrap - flex run-time support library "yywrap" function */

/* $NetBSD: libyywrap.c,v 1.6 2002/01/31 22:43:54 tv Exp $ */

#include <sys/cdefs.h>

#if HAVE_CONFIG_H
#include <config.h>
#endif

int yywrap __P((void));

int
yywrap()
	{
	return 1;
	}
