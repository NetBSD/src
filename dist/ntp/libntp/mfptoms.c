/*	$NetBSD: mfptoms.c,v 1.2 2003/12/04 16:23:37 drochner Exp $	*/

/*
 * mfptoms - Return an asciized signed long fp number in milliseconds
 */
#include "ntp_fp.h"
#include "ntp_stdlib.h"

char *
mfptoms(
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

	return dolfptoa(fpi, fpf, isneg, ndec, 1);
}
