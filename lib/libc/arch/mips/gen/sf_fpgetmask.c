/*	$NetBSD: sf_fpgetmask.c,v 1.3 2000/02/22 03:14:20 mycroft Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

int _mips_sfp_getmask __P((void));

fp_except
fpgetmask()
{
	return _mips_sfp_getmask();
}
