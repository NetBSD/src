/*	$NetBSD: Lint_htonl.c,v 1.1 1997/11/06 00:51:19 cgd Exp $	*/

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
