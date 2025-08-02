/* $NetBSD: Lint_fpsetsticky.c,v 1.2.116.1 2025/08/02 05:54:36 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_except
fpsetsticky(fp_except s)
{
	fp_except rv = { 0 };

	return rv;
}
