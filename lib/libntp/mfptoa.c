/*	$NetBSD: mfptoa.c,v 1.2 1998/01/09 03:16:20 perry Exp $	*/

/*
 * mfptoa - Return an asciized representation of a signed long fp number
 */
#include "ntp_fp.h"
#include "ntp_stdlib.h"

char *
mfptoa(fpi, fpf, ndec)
	u_long fpi;
	u_long fpf;
	int ndec;
{
	int isneg;

	if (M_ISNEG(fpi, fpf)) {
		isneg = 1;
		M_NEG(fpi, fpf);
	} else
		isneg = 0;

	return dolfptoa(fpi, fpf, isneg, ndec, 0);
}
