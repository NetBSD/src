/*	$NetBSD: sf_fpsetmask.c,v 1.2 1999/12/26 00:22:32 shin Exp $	*/

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

	old = _mips_sfp_getmask();
	_mips_sfp_setmask(mask);
	return old;
}
