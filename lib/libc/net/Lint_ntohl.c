/*	$NetBSD: Lint_ntohl.c,v 1.2 1999/05/03 13:12:34 christos Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef ntohl

/*ARGSUSED*/
in_addr_t
ntohl(net32)
	in_addr_t net32;
{
	return (0);
}
