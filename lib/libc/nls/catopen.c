/*	$NetBSD: catopen.c,v 1.4 1995/02/27 13:06:47 cgd Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_catopen,catopen);
#else

#include <nl_types.h>

extern nl_catd _catopen __P((__const char *, int));

nl_catd
catopen(name, oflag)
	__const char *name;
	int oflag;
{
	return _catopen(name, oflag);
}

#endif
