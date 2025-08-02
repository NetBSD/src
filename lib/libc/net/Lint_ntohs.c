/* $NetBSD: Lint_ntohs.c,v 1.4.112.1 2025/08/02 05:54:39 perseant Exp $ */

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
