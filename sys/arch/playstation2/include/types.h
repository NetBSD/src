/*	$NetBSD: types.h,v 1.1.4.3 2002/02/28 04:11:20 nathanw Exp $	*/

#include <mips/types.h>

#define	__HAVE_GENERIC_SOFT_INTERRUPTS

#if defined(_KERNEL) && defined(MIPS3_5900)
typedef int int128_t __attribute__((__mode__(TI)));
typedef unsigned int u_int128_t __attribute__((__mode__(TI)));
#endif /* _KERNEL && MIPS3_5900 */
