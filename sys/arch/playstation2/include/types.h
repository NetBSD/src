/*	$NetBSD: types.h,v 1.2.64.1 2006/11/18 21:29:28 ad Exp $	*/

#include <mips/types.h>

#define	__HAVE_GENERIC_SOFT_INTERRUPTS
#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
