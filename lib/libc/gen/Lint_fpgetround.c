/*	$NetBSD: Lint_fpgetround.c,v 1.1 1997/11/06 00:50:51 cgd Exp $	*/

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_rnd
fpgetround()
{
	fp_rnd rv = { 0 };

	return (rv);
}
