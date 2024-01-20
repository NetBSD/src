/* $NetBSD: Lint_flt_rounds.c,v 1.4 2024/01/20 14:52:47 christos Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */
#include <float.h>

/*ARGSUSED*/
int
__flt_rounds(void)
{
	return (0);
}
