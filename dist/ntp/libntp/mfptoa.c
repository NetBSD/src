/*	$NetBSD: mfptoa.c,v 1.1.1.1 2000/03/29 12:38:50 simonb Exp $	*/

/*
 * mfptoa - Return an asciized representation of a signed long fp number
 */
#include "ntp_fp.h"
#include "ntp_stdlib.h"

char *
mfptoa(
	u_long fpi,
	u_long fpf,
	int ndec
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
