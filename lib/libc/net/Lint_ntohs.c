/* $NetBSD: Lint_ntohs.c,v 1.3.4.1 2001/10/08 20:20:04 nathanw Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef ntohs

/*ARGSUSED*//*NOSTRICT*/
uint16_t
ntohs(net16)
	uint16_t net16;
{
	return (0);
}
