/*	$NetBSD: putenv.c,v 1.1.1.2 2002/06/20 10:30:12 itojun Exp $	*/

#ifndef LINT
static const char rcsid[] = "Id: putenv.c,v 8.4 1999/10/13 16:39:21 vixie Exp";
#endif

#include "port_before.h"
#include "port_after.h"

/*
 * To give a little credit to Sun, SGI,
 * and many vendors in the SysV world.
 */

#if !defined(NEED_PUTENV)
int __bindcompat_putenv;
#else
int
putenv(char *str) {
	char *tmp;

	for (tmp = str; *tmp && (*tmp != '='); tmp++)
		;

	return (setenv(str, tmp, 1));
}
#endif
