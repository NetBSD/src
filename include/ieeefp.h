/*	$NetBSD: ieeefp.h,v 1.6 2005/02/03 04:39:32 perry Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _IEEEFP_H_
#define _IEEEFP_H_

#include <sys/cdefs.h>
#include <machine/ieeefp.h>

fp_rnd    fpgetround(void);
fp_rnd    fpsetround(fp_rnd);
fp_except fpgetmask(void);
fp_except fpsetmask(fp_except);
fp_except fpgetsticky(void);
fp_except fpsetsticky(fp_except);

#endif /* _IEEEFP_H_ */
