/*	$NetBSD: sf_fpsetmask.c,v 1.4 2000/02/22 03:28:04 mycroft Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

int _mips_sfp_getmask __P((void));
void _mips_sfp_setmask __P((int mask));

fp_except
fpsetmask(mask)
	fp_except mask;
{
	fp_except old;

	old = _mips_sfp_getmask();
	_mips_sfp_setmask(mask);
	return old;
}
