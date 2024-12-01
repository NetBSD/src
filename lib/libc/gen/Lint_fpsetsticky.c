/* $NetBSD: Lint_fpsetsticky.c,v 1.3 2024/12/01 16:16:56 rillig Exp $ */

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
