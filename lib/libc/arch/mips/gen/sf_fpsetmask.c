/*	$NetBSD: sf_fpsetmask.c,v 1.1.1.1 1999/09/16 12:18:26 takemura Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_except
fpsetmask(mask)
	fp_except mask;
{
	fp_except old;
	fp_except new;

	old = sfp_getmask();
	sfp_setmask(mask);
	return old;
}
