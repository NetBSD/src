/*	$NetBSD: Lint_htonl.c,v 1.1.2.2 1997/11/08 21:59:05 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>

/*ARGSUSED*/
in_addr_t
htonl(host32)
	in_addr_t host32;
{
	return (0);
}
