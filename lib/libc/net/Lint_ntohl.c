/* $NetBSD: Lint_ntohl.c,v 1.3 2000/06/14 06:49:07 cgd Exp $ */

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
