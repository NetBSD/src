/*	$NetBSD: mfptoa.c,v 1.1.1.1 2009/12/13 16:55:03 kardel Exp $	*/

/*
 * mfptoa - Return an asciized representation of a signed long fp number
 */
#include "ntp_fp.h"
#include "ntp_stdlib.h"

char *
mfptoa(
	u_long fpi,
	u_long fpf,
	short ndec
	)
{
	int isneg;

	if (M_ISNEG(fpi, fpf)) {
		isneg = 1;
		M_NEG(fpi, fpf);
	} else
	    isneg = 0;

	return dolfptoa(fpi, fpf, isneg, ndec, 0);
}
