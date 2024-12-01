/* $NetBSD: Lint_htonl.c,v 1.5 2024/12/01 16:16:56 rillig Exp $ */

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
