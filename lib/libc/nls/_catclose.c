/*	$NetBSD: _catclose.c,v 1.6 2005/07/30 15:21:20 christos Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _catclose.c,v 1.6 2005/07/30 15:21:20 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#if defined(__indr_reference) && !defined(__lint__)
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
