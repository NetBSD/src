/* $NetBSD: Lint_htonl.c,v 1.2.6.1 2000/06/23 16:17:32 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef htonl

/*ARGSUSED*/
in_addr_t
htonl(host32)
	in_addr_t host32;
{
	return (0);
}
