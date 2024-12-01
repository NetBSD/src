/* $NetBSD: Lint_ntohs.c,v 1.5 2024/12/01 16:16:56 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef ntohs

/*ARGSUSED*/
uint16_t
ntohs(uint16_t net16)
{
	return 0;
}
