/*	$NetBSD: ieee.h,v 1.5 2005/04/15 22:39:11 kleink Exp $	*/

#include <sys/ieee754.h>

/*
 * A NaN is a `signalling NaN' if its QUIETNAN bit is clear in its
 * high fraction; if the bit is set, it is a `quiet NaN'.
 */

#if 0
#define	SNG_QUIETNAN	(1 << 22)
#define	DBL_QUIETNAN	(1 << 19)
#endif
