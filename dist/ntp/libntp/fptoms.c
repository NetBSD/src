/*	$NetBSD: fptoms.c,v 1.1.1.1 2000/03/29 12:38:49 simonb Exp $	*/

/*
 * fptoms - return an asciized s_fp number in milliseconds
 */
#include "ntp_fp.h"

char *
fptoms(
	s_fp fpv,
	int ndec
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
