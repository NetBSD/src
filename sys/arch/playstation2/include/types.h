/*	$NetBSD: types.h,v 1.3.18.1 2007/10/03 19:24:39 garbled Exp $	*/

#include <mips/types.h>

#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
