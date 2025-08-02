/* $NetBSD: Lint_htonl.c,v 1.4.112.1 2025/08/02 05:54:39 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef htonl

/*ARGSUSED*/
uint32_t
htonl(uint32_t host32)
{
	return 0;
}
