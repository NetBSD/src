/*	$NetBSD: sf_fpsetsticky.c,v 1.4 2000/02/22 03:28:05 mycroft Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

int _mips_sfp_getsticky __P((void));
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
