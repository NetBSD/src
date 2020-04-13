/*	$NetBSD: nl_langinfo.c,v 1.1.6.2 2020/04/13 07:45:36 martin Exp $	*/

/*
 * Written by Martin Husemann <martin@NetBSD.org>
 * Public domain.
 */

#include <langinfo.h>

/*
 * Cheap and dirty nl_langinfo() - implements just enough
 * for our libcurses in crunched environments.
 */
char *
nl_langinfo(nl_item what)
{
	if (what == CODESET)
		return "ASCII";
	return "";
}
