/* $NetBSD: Lint_ntohl.c,v 1.3.4.1 2001/10/08 20:20:03 nathanw Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef ntohl

/*ARGSUSED*/
uint32_t
ntohl(net32)
	uint32_t net32;
{
	return (0);
}
