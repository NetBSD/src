/*	$NetBSD: ieee.h,v 1.3.14.1 2005/04/19 12:31:03 tron Exp $	*/

#include <sys/ieee754.h>

/*
 * A NaN is a `signalling NaN' if its QUIETNAN bit is set in its
 * high fraction; if the bit is clear, it is a `quiet NaN'.
 */

#if 0
#define	SNG_QUIETNAN	(1 << 22)
#define	DBL_QUIETNAN	(1 << 19)
#endif
