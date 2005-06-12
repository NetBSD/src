/*	$NetBSD: _catgets.c,v 1.6 2005/06/12 05:21:27 lukem Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: _catgets.c,v 1.6 2005/06/12 05:21:27 lukem Exp $");
#endif /* LIBC_SCCS and not lint */

#ifdef __indr_reference
__indr_reference(_catgets,catgets)
#else

#include <nl_types.h>
char	*_catgets __P((nl_catd, int, int, const char *));	/* XXX */

char *
catgets(catd, set_id, msg_id, s)
	nl_catd catd;
	int set_id;
	int msg_id;
	const char *s;
{
	return _catgets(catd, set_id, msg_id, s);
}

#endif
