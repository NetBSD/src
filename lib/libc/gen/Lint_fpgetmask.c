/*	$NetBSD: Lint_fpgetmask.c,v 1.1 1997/11/06 00:50:49 cgd Exp $	*/

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
