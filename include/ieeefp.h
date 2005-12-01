/*	$NetBSD: ieeefp.h,v 1.5.10.1 2005/12/01 16:27:50 riz Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _IEEEFP_H_
#define _IEEEFP_H_

#include <sys/cdefs.h>
#include <machine/ieeefp.h>

__BEGIN_DECLS
fp_rnd    fpgetround __P((void));
fp_rnd    fpsetround __P((fp_rnd));
fp_except fpgetmask __P((void));
fp_except fpsetmask __P((fp_except));
fp_except fpgetsticky __P((void));
fp_except fpsetsticky __P((fp_except));
__END_DECLS

#endif /* _IEEEFP_H_ */
