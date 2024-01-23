/*	$NetBSD: ieee.h,v 1.7 2024/01/23 04:15:54 rin Exp $	*/

#ifndef _POWERPC_IEEE_H_
#define _POWERPC_IEEE_H_

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

#endif /* !_POWERPC_IEEE_H_ */
