/*	$NetBSD: sf_fpgetmask.c,v 1.2 1999/12/26 00:22:31 shin Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_except
fpgetmask()
{
	return _mips_sfp_getmask();
}
