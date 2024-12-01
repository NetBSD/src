/* $NetBSD: Lint_htons.c,v 1.5 2024/12/01 16:16:56 rillig Exp $ */

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
