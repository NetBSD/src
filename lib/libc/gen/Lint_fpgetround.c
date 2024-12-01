/* $NetBSD: Lint_fpgetround.c,v 1.4 2024/12/01 16:16:56 rillig Exp $ */

/*
 * This file placed in the public domain.
 * Chris Demetriou, November 5, 1997.
 */

#include <ieeefp.h>

fp_rnd
fpgetround(void)
{
	fp_rnd rv = { 0 };

	return rv;
}
