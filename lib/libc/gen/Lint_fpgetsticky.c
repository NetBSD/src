/* $NetBSD: Lint_fpgetsticky.c,v 1.3.44.1 2025/08/02 05:54:36 perseant Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

fp_except
fpgetsticky(void)
{
	fp_except rv = { 0 };

	return rv;
}
