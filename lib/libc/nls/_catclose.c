/*	$NetBSD: _catclose.c,v 1.2 1997/07/13 19:58:34 christos Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_catclose,catclose);
#else

#include <nl_types.h>

int
catclose(catd)
	nl_catd catd;
{
	return _catclose(catd);
}

#endif
