/*	$NetBSD: Lint_fpgetsticky.c,v 1.1.2.2 1997/11/08 21:58:19 veego Exp $	*/

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
