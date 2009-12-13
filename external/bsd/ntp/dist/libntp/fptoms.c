/*	$NetBSD: fptoms.c,v 1.1.1.1 2009/12/13 16:55:02 kardel Exp $	*/

/*
 * fptoms - return an asciized s_fp number in milliseconds
 */
#include "ntp_fp.h"

char *
fptoms(
	s_fp fpv,
	short ndec
	)
{
	u_fp plusfp;
	int neg;

	if (fpv < 0) {
		plusfp = (u_fp)(-fpv);
		neg = 1;
	} else {
		plusfp = (u_fp)fpv;
		neg = 0;
	}

	return dofptoa(plusfp, neg, ndec, 1);
}
