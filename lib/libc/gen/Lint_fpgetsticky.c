/* $NetBSD: Lint_fpgetsticky.c,v 1.1.10.1 2000/06/23 16:17:23 minoura Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_except
fpgetsticky()
{
	fp_except rv = { 0 };

	return (rv);
}
