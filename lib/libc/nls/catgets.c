/*	$NetBSD: catgets.c,v 1.6 1995/02/27 13:06:39 cgd Exp $	*/

/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_catgets,catgets);
#else

#include <nl_types.h>

extern char * _catgets __P((nl_catd, int, int, char *));

char *
catgets(catd, set_id, msg_id, s)
	nl_catd catd;
	int set_id;
	int msg_id;
	char *s;
{
	return _catgets(catd, set_id, msg_id, s);
}

#endif
