/* $NetBSD: Lint_fpgetmask.c,v 1.1.10.1 2000/06/23 16:17:22 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_except
fpgetmask()
{
	fp_except rv = { 0 };

	return (rv);
}
