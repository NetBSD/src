/*	$NetBSD: Lint_fpgetsticky.c,v 1.1 1997/11/06 00:50:53 cgd Exp $	*/

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
