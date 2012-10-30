/* $NetBSD: Lint_fpgetround.c,v 1.2.62.1 2012/10/30 18:58:44 yamt Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

/*ARGSUSED*/
fp_rnd
fpgetround(void)
{
	fp_rnd rv = { 0 };

	return (rv);
}
