/* $NetBSD: Lint_htonl.c,v 1.3.4.1 2001/10/08 20:20:02 nathanw Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef htonl

/*ARGSUSED*/
uint32_t
htonl(host32)
	uint32_t host32;
{
	return (0);
}
