/*	$NetBSD: Lint_fpsetmask.c,v 1.1.2.2 1997/11/08 21:58:22 veego Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_except
fpsetmask(m)
	fp_except m;
{
	fp_except rv = { 0 };

	return (rv);
}
