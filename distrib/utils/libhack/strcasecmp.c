/*	$NetBSD: strcasecmp.c,v 1.1 2019/07/28 10:21:18 martin Exp $	*/

/*
 * Written by Martin Husemann <martin@NetBSD.org>
 * Public domain.
 */

#include <strings.h>

/*
 * Cheap and dirty strcasecmp() - implements just enough
 * for our libcurses in crunched environments: since we
 * know all compared strings are fixed, uppercase, and plain ASCII,
 * just use strcmp()
 */
int
strcasecmp(const char *s1, const char *s2)
{
	return strcmp(s1, s2);
}
