/*	$NetBSD: ieeefp.h,v 1.5 2000/06/13 01:21:53 simonb Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _IEEEFP_H_
#define _IEEEFP_H_

#include <sys/cdefs.h>
#include <machine/ieeefp.h>

fp_rnd    fpgetround __P((void));
fp_rnd    fpsetround __P((fp_rnd));
fp_except fpgetmask __P((void));
fp_except fpsetmask __P((fp_except));
fp_except fpgetsticky __P((void));
fp_except fpsetsticky __P((fp_except));

#endif /* _IEEEFP_H_ */
