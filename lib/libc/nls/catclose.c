/*	$NetBSD: catclose.c,v 1.5 1995/03/01 08:00:12 jtc Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_catclose,catclose);
#else

#include <nl_types.h>

extern int _catclose __P((nl_catd));

int
catclose(catd)
	nl_catd catd;
{
	return _catclose(catd);
}

#endif
