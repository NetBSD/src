/*	$NetBSD: sf_fpgetsticky.c,v 1.3 2000/02/22 03:14:21 mycroft Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

int _mips_sfp_getsticky __P((void));

fp_except
fpgetsticky()
{
	return _mips_sfp_getsticky();
}
