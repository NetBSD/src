/*	$NetBSD: _catgets.c,v 1.5 1997/11/04 23:52:49 thorpej Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

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
