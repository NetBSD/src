/*	$NetBSD: sf_fpsetsticky.c,v 1.3 2000/02/22 03:14:22 mycroft Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

void _mips_sfp_setsticky __P((int except));

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;

	old = _mips_sfp_getsticky();
	_mips_sfp_setsticky(sticky);
	return old;
}
