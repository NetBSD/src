/*	$NetBSD: _catclose.c,v 1.3.2.1 1997/11/04 23:54:33 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_catclose,catclose)
#else

#include <nl_types.h>
int	_catclose __P((nl_catd));	/* XXX */

int
catclose(catd)
	nl_catd catd;
{
	return _catclose(catd);
}

#endif
