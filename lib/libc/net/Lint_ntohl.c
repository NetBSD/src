/* $NetBSD: Lint_ntohl.c,v 1.2.6.1 2000/06/23 16:17:33 minoura Exp $ */

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
