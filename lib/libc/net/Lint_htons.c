/* $NetBSD: Lint_htons.c,v 1.4.112.1 2025/08/02 05:54:39 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <sys/types.h>
#undef htons

/*ARGSUSED*/
uint16_t
htons(uint16_t host16)
{
	return 0;
}
