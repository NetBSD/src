/*	$NetBSD: catclose.c,v 1.4 1995/02/27 13:06:30 cgd Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_catclose,catclose);
#else

#include <nl_types.h>

extern void _catclose __P((nl_catd));

void
catclose(catd)
	nl_catd catd;
{
	_catclose(catd);
}

#endif
