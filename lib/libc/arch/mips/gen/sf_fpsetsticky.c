/*	$NetBSD: sf_fpsetsticky.c,v 1.2 1999/12/26 00:22:32 shin Exp $	*/

/*
 * Written by J.T. Conklin, Apr 11, 1995
 * Public domain.
 */

#include <ieeefp.h>

fp_except
fpsetsticky(sticky)
	fp_except sticky;
{
	fp_except old;

	old = _mips_sfp_getsticky();
	_mips_sfp_setsticky(sticky);
	return old;
}
