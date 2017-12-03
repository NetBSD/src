/*	$NetBSD: ieee.h,v 1.5.122.1 2017/12/03 11:36:37 jdolecek Exp $	*/

#include <sys/ieee754.h>

/*
 * A NaN is a `signalling NaN' if its QUIETNAN bit is clear in its
 * high fraction; if the bit is set, it is a `quiet NaN'.
 */

#if 0
#define	SNG_QUIETNAN	(1 << 22)
#define	DBL_QUIETNAN	(1 << 19)
#endif

union ldbl_u {
	long double	ldblu_ld;
	double		ldblu_d[2];
};
