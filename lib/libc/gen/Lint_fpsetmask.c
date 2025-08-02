/* $NetBSD: Lint_fpsetmask.c,v 1.2.116.1 2025/08/02 05:54:36 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_except
fpsetmask(fp_except m)
{
	fp_except rv = { 0 };

	return rv;
}
